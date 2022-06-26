#include <iostream>
#include <future>
#include <vector>
#include <string>
#include <regex>
#include <fstream> // for _complete_ ifstream
#include <stdexcept>
#include <cassert>
#include <cmath>
using std::string;
using std::vector;
using std::runtime_error;
using std::cout;
using std::future;

//** safe conversion of string to number */
template<typename T> inline bool str2num(const string &str, T &val) {
	std::stringstream ss; 		// note: uses "classic" C locale by default
	string sentry;
	string dummy;
	if (str.find('_') != str.npos)
		return false;			// string contains sentry character (which is never valid in a convertible number)
	ss << str << "_"; 			// append sentry character
	bool f1 = (ss >> val);		// read number, check whether successful
	bool f2 = (ss >> sentry);	// check for trailing characters (sentry: OK)
	return ((sentry == "_") && f1 && f2);
}

/** read file contents into string */
vector<string> file2str(string filename) {
	auto inStr = std::ifstream(filename, std::ios::binary);
	if (!inStr.good())
		throw runtime_error("failed to open file " + filename);
	std::ostringstream all;
	all << inStr.rdbuf();
	const string f = all.str();
	static const std::regex r = std::regex("\r?\n");
	return {
		std::sregex_token_iterator(f.begin(), f.end(), r, -1),
		/*equiv. to end()*/std::sregex_token_iterator()};
}

//* Read binary data into vector */
template<class T> vector<T> file2vec(const string &fname) {
	std::ifstream is(fname, std::ifstream::binary);
	if (!is)
		throw runtime_error("failed to open file " + fname);

	is.seekg(0, std::ios_base::end);
	std::size_t nBytes = is.tellg();
	size_t nElem = nBytes / sizeof(T);
	if (nElem * sizeof(T) != nBytes)
		throw runtime_error("file contains partial element");

	is.seekg(0, std::ios_base::beg);
	vector<T> retVal(nElem);
	is.read((char*) &retVal[0], nBytes);
	if (!is)
		throw runtime_error("read failed");
	return retVal;
}

/** "and" operation between two bool vectors */
vector<bool> logicalAnd(const vector<bool> &arg1, const vector<bool> &arg2) {
	// === empty vector: Default value (returns other argument) ===
	if (arg1.size() == 0)
		return arg2;
	if (arg2.size() == 0)
		return arg1;

	assert(arg1.size() == arg2.size());
	vector<bool> retVal;
	retVal.reserve(arg1.size());
	auto it1 = arg1.cbegin();
	auto it1end = arg1.cend();
	auto it2 = arg2.cbegin();
	for (; it1 != it1end; ++it1, ++it2)
		retVal.push_back(*it1 & *it2);
	return retVal;
}

/** returns number of true elements in indexOp */
inline size_t popcount(const vector<bool> &indexOp) {
	return std::count(indexOp.cbegin(), indexOp.cend(), true); // should ideally use popcnt intrinsic (reportedly, the C++ 20 implementation does)
}

/** one bit field per DUT; true: passes; false: fails */
vector<bool> calcPassFailMask(const string &fname, float lowLim, float highLim) {
	vector<float> data = file2vec<float>(fname);
	vector<bool> result;
	result.reserve(data.size());

	for (auto v : data)
		result.push_back(std::isnan(v) || ((v >= lowLim) && (v <= highLim))); // nan: missing data (test does not exist in one .stdf file among several) => pass
	return result;
}

int main() {
	// === file to string ===
	vector<string> lines = file2str("examples/myLimits.txt");
	typedef vector<bool> failMask_t;

	// iterate over limits ===
	vector<future<failMask_t>> evalResults; // multi-threaded results
	for (auto line : lines) {
		// === split by separator ===
		// example uses comma
		static const std::regex r = std::regex(",");
		vector<string> fields(std::sregex_token_iterator(line.begin(), line.end(), r, -1),/*equiv. to end()*/std::sregex_token_iterator());

		// === identify valid limit ===
		// example checks for 3 elements that are convertible to int, float, float
		if (fields.size() != 3)
			continue;
		int testnum;
		if (!str2num(fields[0], testnum))
			continue;
		float lowLim;
		if (!str2num(fields[1], lowLim))
			continue;
		float highLim;
		if (!str2num(fields[2], highLim))
			continue;
		// cout << testnum << "\t" << lowLim << "\t" << highLim << "\n";

		// === run limits check in the background ===
		// parallelizes slow loading of large data files
		string fname = "outSmall/" + std::to_string(testnum) + ".float";
		evalResults.push_back(std::async(calcPassFailMask, fname, lowLim, highLim));
	}

	// combined pass/fail vector
	vector<bool> rTot;
	for (size_t ix = 0; ix < evalResults.size(); ++ix)
		rTot = logicalAnd(rTot, evalResults[ix].get());

	cout << "nPass:\t" << popcount(rTot) << "\n";
	cout << "nTot:" << rTot.size() << "\n";

	return /*EXIT_SUCCESS*/0;
}
