// with recent compiler (default: C++17 or up)
// g++ -O3 -DNDEBUG -o STDFoo.exe STDFoo.cpp -lz
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <zlib.h>
#include <string>
#include <cassert>
#include <stdint.h>
#include <stdlib.h>
using std::string;
using std::cerr;
using std::cout;
using std::endl;
void fail(const char *msg) {
	cerr << msg << endl;
	cerr << "exiting" << endl;
	exit(EXIT_FAILURE);
}

//* extracts T from ptr (no alignment required) */
template<class T> static T decode(unsigned char *&ptr) {
	T retval;
	unsigned char *dest = (unsigned char*) &retval;
	for (unsigned int ix = 0; ix < sizeof(T); ++ix) {
		*(dest++) = *(ptr++);
	}
	return retval;
}

// ==========================
// === Directory creation ===
// ==========================
// Note: could omit the std::filesystem variant entirely as POSIX works just fine but it seems cleaner in the long run
#ifdef PRE_CPP17
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
static void createDirectory(string dirname) {
	struct stat st = { 0 };
	if (stat(dirname.c_str(), &st) == -1) {
		int val = mkdir(dirname.c_str());
		if (val != 0){
			fail("directory creation failed");
		}
	}
}
#else
#include <filesystem>
static void createDirectory(string dirname) {
	// Note: this function is available with C++17 and up. Some compilers may need additional libraries
	// e.g. -lstdc++fs or -lc++fs.
	std::filesystem::create_directory(dirname);
}
#endif

// ===============
// === circBuf ===
// ===============
/** fast circular buffer (near copy-free) with guaranteed contiguous readback chunk size (so that a full STDF record with max. 16 bit length plus header can be read at once) */
class circBuf {
public:
	circBuf(unsigned int nCirc, unsigned int nContigRead) {
		assert(nCirc >= nContigRead);
		this->nCirc = nCirc;
		this->nContigRead = nContigRead;
		buf = (unsigned char*) malloc(this->nCirc + this->nContigRead - 1); // TBD use C++...
		if (!buf)
			throw new std::runtime_error("malloc failed");
		this->nData = 0;
		this->ixPush = 0;
		this->ixPop = 0;
	}

	//* returns max. number of bytes to input into readDest, to be reported afterwards via reportPush() */
	void getLargestPossiblePush(unsigned int *nBytesMax, unsigned char **dest) {
		// limited by remaining circular capacity
		unsigned int n = this->nCirc - this->nData;
		// in addition, don't overrun the buffer end by more than the excess capacity (which is nContigRead)
		n = std::min(n, this->nCirc + this->nContigRead - 1 - this->ixPush);

		*nBytesMax = n;
		*dest = this->buf + this->ixPush;
	}

	/** after preparing data entry with "getLargestPossiblePush" and copying the data, register the data here */
	void reportPush(unsigned int n) {
		assert(n <= this->nCirc - this->nData);
		assert(n <= this->nCirc + this->nContigRead - 1 - this->ixPush);

		if (this->ixPush < this->nContigRead) {
			// we wrote to the head of the regular buffer.
			//Replicate into the circular extension.
			int nCopy = this->nContigRead - this->ixPush;
			memcpy(/*dest*/this->buf + this->nCirc + this->ixPush, /*src*/
			this->buf + this->ixPush, /*size*/nCopy);
		}

		this->ixPush += n; // points now to end of new data

		if (this->ixPush > this->nCirc) {
			// did we write at least one byte into the excess capacity?
			unsigned int nExcess = this->ixPush - this->nCirc;
			// Duplicate excess write to start of buffer (it still remains in excess capacity, enabling read beyond
			// the circular buffer size)
			memcpy(/*dest*/this->buf, /*src*/this->buf + this->nCirc, /*size*/
			nExcess);
		}
		// did we reach the end of the circular buffer?
		if (this->ixPush >= this->nCirc) {
			this->ixPush -= this->nCirc;
		}
		this->nData += n;
	}

	//* returns max. number of bytes to take from data, to be reported afterwards via reportPop() */
	void getLargestPossiblePop(unsigned int *nBytesMax, unsigned char **src) {
		*nBytesMax = std::min(this->nData, this->nContigRead);
		*src = this->buf + this->ixPop;
	}

	//* after using the data acquired by "getLargestPossiblePop()", remove part or all of it */
	void pop(unsigned int n) {
		assert(n <= this->nData);
		this->ixPop += n;

		// did we reach the end of the regular buffer?
		if (this->ixPop >= this->nCirc) {
			// continue from the replica at the buffer head
			this->ixPop -= this->nCirc;
		}
		this->nData -= n;
	}
protected:
	/** circular buffer */
	unsigned char *buf;
	/** size of the circular buffer (maximum read-ahead) */
	unsigned int nCirc;
	/** maximum read size to retrieve at one time*/
	unsigned int nContigRead;
	/** number of bytes in buffer */
	unsigned int nData;

	/** position of next push */
	unsigned int ixPush;
	/** position of next pop */
	unsigned int ixPop;
	~circBuf() {
		free(this->buf);
	}
};

// =======================
// === blockingCircBuf ===
// =======================
/** multithreading layer over circBuf for parallel data input / output */
class blockingCircBuf: circBuf {
public:
	blockingCircBuf(unsigned int nCirc, unsigned int nContigRead) :
			circBuf(nCirc, nContigRead) {
		this->isShutdown = false;
	}

	/* allows push of up to nBytesMax (which will be at least nBytesMin). Returns true if shutdown. */
	bool getLargestPossiblePush(unsigned int nBytesMin, unsigned int *nBytesMax,
			unsigned char **readDest) {
		assert(nBytesMin <= this->nContigRead);
		std::unique_lock<std::mutex> lk(this->m);
		while (true) {
			if (this->isShutdown)
				return true;
			this->circBuf::getLargestPossiblePush(nBytesMax, readDest);
			if (*nBytesMax >= nBytesMin)
				return false;
			cvPop.wait(lk);
		}
	}
	void reportPush(unsigned int n) {
		std::lock_guard<std::mutex> lk(this->m);
		this->circBuf::reportPush(n);
		this->cvPush.notify_one();
	}
	/** blocks until at least nBytesMin are available. Returns true (eos) in shutdown once all data has been consumed (non-zero nBytesMax < nBytesMin if trailing bytes) */
	bool getLargestPossiblePop(unsigned int nBytesMin, unsigned int *nBytesMax,
			unsigned char **readDest) {
		assert(nBytesMin <= this->nContigRead);
		std::unique_lock<std::mutex> lk(this->m);
		while (true) {
			this->circBuf::getLargestPossiblePop(nBytesMax, readDest);
			if (*nBytesMax >= nBytesMin)
				return false; // note: even on shutdown, keep delivering data until empty
			if (this->isShutdown)
				return true; //
			cvPush.wait(lk);
		}
		return true;
	}
	void pop(unsigned int n) {
		std::lock_guard<std::mutex> lk(this->m);
		this->circBuf::pop(n);
		cvPop.notify_one();
	}
	void setShutdown(bool shutdown) {
		std::lock_guard<std::mutex> lk(this->m);
		this->isShutdown = shutdown;
		if (shutdown) {
			cvPush.notify_one();
			cvPop.notify_one();
		}
	}
protected:
	/** thread protection of all internals */
	std::mutex m;
	/** wakeup call on push */
	std::condition_variable cvPush;
	/** wakeup call on pop */
	std::condition_variable cvPop;
	/** indicates that no new data will arrive (and no new data will be accepted) */
	bool isShutdown;
};

// =================
// === doubleBuf ===
// =================
//** collects data to be written to a file in the background (main motivation: to deal with more files than available filehandles e.g. 2048 on Windows 8.1) */
template<class T> class doubleBuf {
public:
	doubleBuf(string filename) {
		this->filename = filename;
	}
	void input(T val) {
		std::lock_guard<std::mutex> lk(this->m);
		this->buffer[this->bufPrimary].push_back(val);
	}

	//* writes contents to file, possibly from an external thread (at most one additional thread). Returns false if idle */
	bool writeToFile() {
		// === open file ===
		std::ofstream fhandle = std::ofstream();
		if (this->createFile) {
			fhandle.open(this->filename,
					std::ofstream::out | std::ofstream::binary);
			// check for empty buffer only _after_ the file was created (which may take some time)
			std::lock_guard<std::mutex> lk(this->m);
			if (this->buffer[this->bufPrimary].size() < 1)
				return false;

		} else {
			// check for empty buffer _before_ opening the file
			std::lock_guard<std::mutex> lk(this->m);
			if (this->buffer[this->bufPrimary].size() < 1)
				return false;
			fhandle.open(this->filename,
					std::ofstream::out | std::ofstream::binary
							| std::ofstream::app);
		}
		if (!fhandle.is_open()) {
			cerr << "Failed to open '" << this->filename << "' for write"
					<< endl;
			fail("");
		}
		this->createFile = false;

		std::vector<T> *b = &this->buffer[this->bufPrimary];

		{ // === swap buffers ===
			std::lock_guard<std::mutex> lk(this->m);
			this->bufPrimary = (this->bufPrimary + 1) & 1;
		} // RAII mutex

		//=== write data ===
		T *pFirstElem = &((*b)[0]);
		fhandle.write((const char*) pFirstElem, b->size() * sizeof(T));
		b->clear();
		fhandle.close();
		return true;
	}
protected:
	/** where to write the data to */
	string filename;
	/** lock concurrent access */
	std::mutex m;
	/** data buffers */
	std::vector<T> buffer[2];
	/** which one of the two buffers is being written into */
	unsigned int bufPrimary = 0;
	/** startup flag */
	bool createFile = true;
};

// =======================
// === perPartLoggable ===
// =======================
//* data logging for one testitem. Manages data validity efficiently using timestamps */
template<class T> class perPartLoggable {
public:
	perPartLoggable(std::string fname, T defVal) :
			buf(fname) {
		this->defVal = defVal;
		this->nWritten = 0;
	}

	/** sets data with timestamp */
	void setData(unsigned int site, unsigned int validCode, T value) {
		this->addSiteIfMissing(site);
		this->sitedata[site] = value;
		this->validCode[site] = validCode;
	}

	/** write this item for given site. Fill missing data in file with default value. */
	void write(unsigned int site, unsigned int dutCountBaseZero,
			unsigned int validCode) {
		this->addSiteIfMissing(site);

		// invalid site: write as invalid value, otherwise write separately
		bool dataIsValid = this->validCode[site] == validCode;

		// === write invalid entries to datalog ===
		unsigned int firstUnpadded =
				dataIsValid ? dutCountBaseZero : dutCountBaseZero + 1;
		while (this->nWritten < firstUnpadded) {
			this->buf.input(this->defVal);
			++this->nWritten;
		}

		// === write valid entry ===
		if (dataIsValid) {
			this->buf.input(this->sitedata[site]);
			++this->nWritten;
		}
	}

	/** optional write-to-file of buffered data from background thread. Returns true if data was written */
	bool flush() {
		return this->buf.writeToFile();
	}

	/** write-to-file from background thread. Possibly redundant with flush() but does no harm. */
	void close() {
		this->buf.writeToFile();
	}

protected:
	void addSiteIfMissing(unsigned int site) {
		if (site >= this->sitedata.size()) {
			this->sitedata.resize(site + 1);
			this->validCode.resize(site + 1, 0); // 0 is never valid
		}
	}

	//* collected data per site */
	std::vector<T> sitedata;

	//* timestamp of data per site */
	std::vector<unsigned int> validCode;

	//* how many DUTs have been recorded (to pad the output file for missing item */
	unsigned int nWritten;

	//* on PRR (binning/results/removal), data is written to buf which eventually writes to file */
	doubleBuf<T> buf;

	//* default value for invalid data */
	T defVal;
};

// ====================
// === commonLogger ===
// ====================
/** writes items common to all DUTs (testnumber, testnames, limits, units). Can afford one filehandle per item. */
class commonLogger {
public:
	commonLogger(string dirname) {
		this->directory = dirname;
	}

	//* checks for first occurrence of testnum PTR
	bool isLogged(unsigned int testnum) {
		// STDF standard: "The first occurrence of this record also establishes the default values for all semi-static information about the test, such as limits, units, and scaling."
		// Performance-critical, as "first occurrence" of testnum needs to be checked for every PTR record.
		return this->loggedTests.find(testnum) != this->loggedTests.end();
	}

	//* records default values from first occurrence of PTR record
	void log(unsigned int testnum, float lowLim, float highLim, string testname,
			string unit) {
		this->lowLim[testnum] = lowLim;
		this->highLim[testnum] = highLim;
		this->testname[testnum] = testname;
		this->unit[testnum] = unit;
		this->loggedTests.insert(testnum);
	}

	//* writes collected data to files */
	void close() {
		// === copy testnums to sorted set ===
		std::set<unsigned int> testnums;
		for (auto it = this->loggedTests.begin(); it != this->loggedTests.end();
				++it) {
			testnums.insert(*it);
		}

		this->openHandle(this->directory + "/testnums.uint32");
		for (auto it = testnums.begin(); it != testnums.end(); ++it) {
			uint32_t tmp = *it;
			this->h.write((const char*) &tmp, sizeof(tmp));
		}
		this->closeHandle();

		this->openHandle(this->directory + "/testnames.txt");
		for (auto it = testnums.begin(); it != testnums.end(); ++it)
			this->h << this->testname[*it] << "\n";
		this->closeHandle();

		this->openHandle(this->directory + "/units.txt");
		for (auto it = testnums.begin(); it != testnums.end(); ++it)
			this->h << this->unit[*it] << "\n";
		this->closeHandle();

		this->openHandle(this->directory + "/lowLim.float");
		for (auto it = testnums.begin(); it != testnums.end(); ++it) {
			float tmp = this->lowLim[*it];
			this->h.write((const char*) &tmp, sizeof(tmp));
		}
		this->closeHandle();

		this->openHandle(this->directory + "/highLim.float");
		for (auto it = testnums.begin(); it != testnums.end(); ++it) {
			float tmp = this->highLim[*it];
			this->h.write((const char*) &tmp, sizeof(tmp));
		}
		this->closeHandle();

		this->openHandle(this->directory + "/filenames.txt");
		for (auto it = this->filenames.begin(); it != this->filenames.end();
				++it)
			this->h << *it << "\n";
		this->closeHandle();

		this->openHandle(this->directory + "/dutsPerFile.uint32");
		for (auto it = this->dutsPerFile.begin(); it != this->dutsPerFile.end();
				++it) {
			uint32_t tmp = *it;
			this->h.write((const char*) &tmp, sizeof(tmp));
		}
		this->closeHandle();

		// human-readable file / lot summary in csv format
		this->openHandle(this->directory + "/filelist.txt");
		this->h << "filename\tnDuts\n";
		for (unsigned int ix = 0; ix < this->filenames.size(); ++ix)
			this->h << this->filenames[ix] << "\t" << this->dutsPerFile[ix]
					<< "\n";
		this->closeHandle();

		// human-readable summary in csv format
		this->openHandle(this->directory + "/testlist.txt");
		this->h << "TEST_NUM\tTEST_TXT\tUNITS\tLO_LIMIT\tHI_LIMIT\n";
		for (auto it = testnums.begin(); it != testnums.end(); ++it) {
			this->h << (*it) << "\t" //
					<< this->testname[*it] << "\t" //
					<< this->unit[*it] << "\t" //
					<< this->lowLim[*it] << "\t" //
					<< this->highLim[*it] << "\n";
		}
		this->closeHandle();
	}
	void reportFile(string filename, unsigned int dutsPerFile) {
		this->filenames.push_back(filename);
		this->dutsPerFile.push_back(dutsPerFile);
	}
protected:
	void openHandle(string fname) {
		this->h.open(fname, std::ofstream::out | std::ofstream::binary);
		if (!this->h.is_open()) {
			cerr << "Failed to open '" << fname << "' for write" << endl;
			fail("");
		}
	}
	void closeHandle() {
		this->h.close();
	}
	std::ofstream h;
	string directory;
	std::unordered_map<unsigned int, float> lowLim;
	std::unordered_map<unsigned int, float> highLim;
	std::unordered_map<unsigned int, string> testname;
	std::unordered_map<unsigned int, string> unit;
	std::unordered_set<unsigned int> loggedTests;
	std::vector<string> filenames;
	std::vector<unsigned int> dutsPerFile;
};

// ==================
// === stdfWriter ===
// ==================
/** takes one input STDF record at a time, extracts detailed data and routes to various writers */
class stdfWriter {
public:
	stdfWriter(string dirname) :
			cmLog(dirname) {
		this->directory = dirname;
		this->nextValidCode = 1; // 0 is "invalid"
		this->loggerSite = new perPartLoggable<uint8_t>(
				dirname + "/" + "site.uint8", 255);
		this->loggerHardbin = new perPartLoggable<uint16_t>(
				dirname + "/" + "hardbin.uint16", 65535);
		this->loggerSoftbin = new perPartLoggable<uint16_t>(
				dirname + "/" + "softbin.uint16", 65535);
		this->dutCountBaseZero = 0;
		this->dutsReported = 0;
	}

	static string decodeString(unsigned char *&ptr) {
		// note: STDF string may use less space than advertised by len byte, if null-terminated
		char buf[255 + 1];
		uint8_t len = *(ptr++);
		for (unsigned int ix = 0; ix < len; ++ix) {
			buf[ix] = *(ptr++);
		}
		buf[len] = 0;
		if (buf[0] == 0)
			return std::string("null");
		else
			return std::string(&buf[0]);
	}

	void stdfRecord(unsigned char *ptr) {
		ptr += 2; // record length
		uint16_t hdr = decode<uint16_t>(ptr);
		switch (hdr) {
		case 0 + (10 << 8): { // FAR
			// do nothing...
			break;
		}
		case 5 + (10 << 8): { // PIR
			// cout << "PIR\n";
			ptr += 1; // HEAD_NUM
			unsigned int SITE_NUM = decode<uint8_t>(ptr);
			this->PIR(SITE_NUM);
			break;
		}
		case 5 + (20 << 8): { // PRR
			// cout << "PRR\n";
			ptr += 1; // HEAD_NUM
			unsigned int SITE_NUM = decode<uint8_t>(ptr);
			ptr += 3; // PART_FLG, NUM_TEST
			unsigned int HARD_BIN = decode<uint16_t>(ptr);
			unsigned int SOFT_BIN = decode<uint16_t>(ptr);
			this->PRR(SITE_NUM, SOFT_BIN, HARD_BIN);
			break;
		}
		case 15 + (10 << 8): { // PTR
			//cout << "PTR\n";
			unsigned int TEST_NUM = decode<uint32_t>(ptr);
			ptr += 1; // HEAD_NUM
			unsigned int SITE_NUM = decode<uint8_t>(ptr);
			ptr += 2; // TEST_FLG, PARM_FLG
			float RESULT = decode<float>(ptr);
			// cout << TEST_NUM << " " << RESULT << endl;
			this->PTR(TEST_NUM, SITE_NUM, RESULT);
			if (!this->cmLog.isLogged(TEST_NUM)) {
				string testtext = decodeString(ptr);
				string alarmId = decodeString(ptr);
				ptr += 4; //OPT_FLAG, RES_SCAL, LLM_SCAL, HLM_SCAL
				float lowLim = decode<float>(ptr);
				float highLim = decode<float>(ptr);
				string unit = decodeString(ptr);
				this->cmLog.log(TEST_NUM, lowLim, highLim, testtext, unit);
			}
			break;
		}
		default: {
			// cerr << "warning: unsupported record (OK outside testcases)" << endl;
		}
		}
	}

	void PIR(unsigned int site) {
		if (this->siteValidCode.size() <= site)
			this->siteValidCode.resize(site + 1);
		if (this->siteValidCode[site]) {
			cerr << "warning: inconsistent file structure. PIR on open site "
					<< site << " (missing PRR)" << endl;
		}
		this->siteValidCode[site] = this->nextValidCode++;
	}

	void PTR(unsigned int testnum, unsigned int site, float val) {
		if (this->siteValidCode.size() <= site)
			this->siteValidCode.resize(site + 1);
		if (!this->siteValidCode[site]) {
			std::cerr
					<< "Warning: inconsistent file structure. PTR on closed site "
					<< site << " (missing PIR)" << endl;
			return;
		}

		// === look up or create data structure ===
		auto it = this->loggerTestitems.find(testnum);
		perPartLoggable<float> *i;
		if (it != this->loggerTestitems.end()) {
			i = it->second;
		} else {
			std::ostringstream tmp;
			tmp << this->directory << "/" << testnum << ".float";
			i = new perPartLoggable<float>(tmp.str(), std::nanf(""));

			this->loggerTestitems[testnum] = i;
		}

		i->setData(site, this->siteValidCode[site], val);
	}

	void PRR(unsigned int site, uint16_t softbin, uint16_t hardbin) {
		if (this->siteValidCode.size() <= site)
			this->siteValidCode.resize(site + 1);
		unsigned int validCode = this->siteValidCode[site];
		if (validCode == 0) {
			std::cerr
					<< "warning: inconsistent file structure. PRR on closed site "
					<< site << " (missing PIR)" << endl;
			return;
		}

		this->loggerSoftbin->setData(site, validCode, softbin);
		this->loggerHardbin->setData(site, validCode, hardbin);
		this->loggerSite->setData(site, validCode, site);

		// === write data ===
		for (auto it = this->loggerTestitems.begin();
				it != loggerTestitems.end(); ++it) {
			(*it).second->write(site, this->dutCountBaseZero, validCode);
		}
		this->loggerSoftbin->write(site, this->dutCountBaseZero, validCode);
		this->loggerHardbin->write(site, this->dutCountBaseZero, validCode);
		this->loggerSite->write(site, this->dutCountBaseZero, validCode);

		this->siteValidCode[site] = 0;

		// note: parts are counted in the order they are reported / removed (PRR)
		++this->dutCountBaseZero;
	}

	bool flush() {
		bool retVal = false;
		for (auto it = this->loggerTestitems.begin();
				it != this->loggerTestitems.end(); ++it)
			retVal |= it->second->flush();
		retVal |= this->loggerSoftbin->flush();
		retVal |= this->loggerHardbin->flush();
		retVal |= this->loggerSite->flush();
		return retVal;
	}

	void close() {
		for (unsigned int ix = 0; ix < this->siteValidCode.size(); ++ix)
			if (this->siteValidCode[ix] != 0)
				std::cerr << "Warning: site " << ix
						<< " has no result (PIR without PRR)\n";

		for (auto it = this->loggerTestitems.begin();
				it != this->loggerTestitems.end(); ++it)
			it->second->close();
		this->loggerSoftbin->close();
		this->loggerHardbin->close();
		this->loggerSite->close();
		this->cmLog.close();
	}

	void reportFile(string filename) {
		this->cmLog.reportFile(filename,
				this->dutCountBaseZero - this->dutsReported);
		this->dutsReported = this->dutCountBaseZero;
	}
	~stdfWriter() {
		for (auto it = this->loggerTestitems.begin();
				it != this->loggerTestitems.end(); ++it) {
			delete it->second;
		}
		delete this->loggerSite;
		delete this->loggerHardbin;
		delete this->loggerSoftbin;
	}
protected:
//* directory common to all written files
	string directory;
//* data loggers per TEST_NUM
	std::unordered_map<unsigned int, perPartLoggable<float>*> loggerTestitems;
//* log NUM_SITE per insertion
	perPartLoggable<uint8_t> *loggerSite;
//* log HARD_BIN per insertion
	perPartLoggable<uint16_t> *loggerHardbin;
//* log SOFT_BIN per insertion
	perPartLoggable<uint16_t> *loggerSoftbin;
//* timestamp to monitor PIR-PTR*n-PRR sequence, also to recognize whether data in loggers is valid (motivation: advancing one timestamp is faster than invalidating thousands of records)
	unsigned int nextValidCode;
//* timestamp per site for PIR-PTR*n-PRR sequence monitoring
	std::vector<unsigned int> siteValidCode;
//* number of insertions = current length of all per-DUT results
	unsigned int dutCountBaseZero;
//* logger for non-per-DUT data e.g. testnames
	commonLogger cmLog;
	unsigned int dutsReported;
};

// =======================
// === pingPongMailbox ===
// =======================
//* minimal communication between two threads using one payload message of type T
template<class T> class pingPongMailbox {
public:
	enum state_e {
		PING, PONG
	};
	state_e state = PING;
	pingPongMailbox() {
	}
	state_e getState() {
		std::lock_guard<std::mutex> lk(this->m);
		return this->state;
	}
	T getPayload() {
		std::lock_guard<std::mutex> lk(this->m);
		return this->payload;
	}
	void wait() {
		std::unique_lock<std::mutex> lk(this->m);
		this->evt.wait(lk);
	}
	//* change of state unlocks other wait()ing thread
	void setState(state_e state, T payload) {
		std::lock_guard<std::mutex> lk(this->m);
		this->state = state;
		this->payload = payload;
		this->evt.notify_all();
	}
protected:
	std::mutex m;
	std::condition_variable evt;
	T payload;
};

void main_reader(string filename, blockingCircBuf &reader) {
	// === feed file ===
	gzFile_s *f = gzopen(filename.c_str(), "rb");
	if (!f) {
		cerr << "failed to open '" << filename << "' for read";
		fail("");
	}
	while (!gzeof(f)) {
		unsigned int nBytesMax;
		unsigned char *dest;
		bool eos = reader.getLargestPossiblePush(/*nBytesMin*/1, &nBytesMax,
				&dest);
		if (eos)
			break;
		unsigned int nRead = gzread(f, (void*) dest, nBytesMax);
		reader.reportPush(nRead);
	} // while not EOF
	cout << "finished " << filename << endl;
	gzclose(f);
}

void main_writer(string filename, blockingCircBuf &reader, stdfWriter &writer) {
	bool startup = true;
	while (true) {
		unsigned char *ptr;
		unsigned int nBytesAvailable;
		bool shutdown = reader.getLargestPossiblePop(2, &nBytesAvailable, &ptr);
		if (shutdown) {
			if (nBytesAvailable > 0)
				cerr << "Warning: " << filename
						<< " has incorrect format (records not complete)"
						<< endl;
			break;
		}

		unsigned char *ptrCopy = ptr; // don't want decode() to advance pointer
		uint16_t recordSize = decode<uint16_t>(ptrCopy);
		unsigned int recordSizeWithHeader = recordSize + 4;
		if (startup) {
			if (recordSize == 0x0200) {
				cerr << "Big Endian STDF file format is not supported" << endl;
				fail("");
			} else if (recordSize != 0x0002) {
				cerr << "invalid STDF file" << endl;
				fail("");
			}
			startup = false;
		}
		while (nBytesAvailable < recordSizeWithHeader) {
			shutdown = reader.getLargestPossiblePop(recordSizeWithHeader,
					&nBytesAvailable, &ptr);
			if (shutdown) {
				if (nBytesAvailable > 0)
					cerr << "Warning: " << filename
							<< " has incorrect format (records not complete)"
							<< endl;
				break;
			}
		}
		writer.stdfRecord((unsigned char*) ptr);
		reader.pop(recordSizeWithHeader);
	} // while true (records in file)
	writer.reportFile(filename);
}

// ============
// === main ===
// ============
int main(int argc, char **argv) {
	cout.precision(9);
	if (argc <= 2) {
		cerr << "usage: " << argv[0] << " outputfolder inputfile.stdf.gz"
				<< endl;
		fail("");
	}
	string dirname(argv[1]);
	createDirectory(dirname);

	// === multithreaded version (recommended) ===
	unsigned int nCirc = 65600 * 128; // max. read-ahead (performance parameter. This number gives best performance on 5 GB testcase)
	unsigned int nChunkMax = 65535 + 4; // max. single pop size. STDF 4-byte header is not included in 16-bit count
	pingPongMailbox<string> mailbox;

	blockingCircBuf reader(nCirc, nChunkMax);
	std::thread readerThread([&argv, &argc, &reader, &mailbox] {
		for (int ixFile = 2; ixFile < argc; ++ixFile) {
			string filename(argv[ixFile]);

			// === wait for downstream processing to finish ===
			// this thread owns the "PING" end of the mailbox
			while (mailbox.getState() != mailbox.PING) {
				mailbox.wait();
			}

			reader.setShutdown(false);

			// === notify downstream processing ===
			mailbox.setState(mailbox.PONG, filename);

			// === feed data ===
			main_reader(filename, reader);
			reader.setShutdown(true);
		}

		while (mailbox.getState() != mailbox.PING) {
			mailbox.wait();
		}

		// === notify downstream processing there is no more data ===
		mailbox.setState(mailbox.PONG, /*agreed protocol: empty string => done*/
		"");
	});

	stdfWriter writer(dirname);
	std::thread recordParserThread([&reader, &writer, &mailbox] {
		while (true) {
			// === wait for news ===
			// this thread owns the "PONG" end of the mailbox
			while (mailbox.getState() != mailbox.PONG) {
				mailbox.wait();
			}
			string filename = mailbox.getPayload();
			if (filename.length() == 0) {
				break;
			}
			main_writer(filename, reader, writer);
			mailbox.setState(mailbox.PING, /*don't-care return payload*/"");
		}
	}
	);

	bool backgroundWriteRunning = true;
	std::thread backgroundWriterThread([&writer, &backgroundWriteRunning] {
		while (backgroundWriteRunning) {
			bool wroteSomeData = writer.flush();
			// suspend if idle (don't go into a spin loop if inbound data is slow).
			// Note: Could use a condition variable signaled on data but sleep() is simple and stupid with minimal overhead.
			if (!wroteSomeData)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	);

	readerThread.join();
	recordParserThread.join();
	reader.setShutdown(true); // redundant unless no files
	backgroundWriteRunning = false;
	backgroundWriterThread.join();
	writer.close();
	return 0;
}
