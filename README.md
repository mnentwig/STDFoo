# STDFoo
a) Converts ATE .stdf(.gz) to binary float data, one file per TEST_NUM.

b) Imports resulting binary data to Octave _efficiently_ (load-on-demand, designed for xy GB-size datasets or beyond)

* used STDF fields are *PIR* (insertion), *PTR* (individual test data), *PRR* (results/binning). All other records are skipped
* pure C++, needs only a recent compiler e.g. from MinGW, no libraries required except libz. 
* Fast: A dataset 832 Mb zipped, 30000 simulated DUTs with 2k testitems each is converted in around 12 s on a 2013 PC. More data scales linearly (fixed mem)
* internally multithreaded, gives 40+ % performance boost
* compatible/future-proof: Compiles with -std=(c++11, c++17, c++20, c++23)
* "Simple and stupid", both usage and build process
* Use of Octave end is optional.

### Command line arguments: 
STDfoo.exe myOutDirectory myInputfile1.stdf.gz myInputfile2.stdf.gz myInputfile3.stdf.gz ...
	
The output directory will be created.

### Results in myOutputDirectory:
* testnums.uint32: all encountered TEST_NUM fields in ascending order
* testnames.txt: newline-separated TEST_DESC strings, one per TEST_NUM
* units.txt: newline-separated UNIT for each TEST_NUM
* lowLim.float: Low limit corresponding to TEST_NUM 
* highLim.float: High limit corresponding to TEST_NUM
* (num).float: One file (32-bit single precision binary) with RESULT of all DUTs, in order of ejection (matches order of PRR records in file). There will be as many files as TEST_NUMs.
* hardbin.u16: The hardbin for each DUT
* softbin.u16: The softbin for each DUT
* site.u8: The site where each DUT was tested
* testlist.txt: a human-readable / csv style summary with test numbers, names, units and limits
* files.txt: list of files from command line
* dutsPerFile.uint32: Number of duts in each file
* fileList.txt: human-readable csv style table with filenames and DUTs per file

### Octave end:
The quickest 'installation' is to simply copy 'STDFoo.m' into the same directory where 'myOutputDirectory' was created
* o=STDFoo('myOutputDirectory') opens a handle into 'myOutputDirectory'. Hint: available functions below show with tab completion for 'o.' in the Octave GUI
* o.DUTs.getDataByTestnum(testnum) per DUT. Returns column vector with RESULT(testnum). Vector input returns one column per testnum.
* o.DUTs.getSite() per DUT. Returns used test site.
* o.DUTs.getHardbin() per DUT. Returns binning information.
* o.DUTs.getSoftbin() same
* o.tests.getTestnums() per test. Testnumber (sorted)
* o.tests.getUnits() per test. Cellarray of all units (per test order matches getTestnums(
* o.tests.getLowLim() per test. Low limits
* o.tests.getHighLim() per test. High limit
* o.getNDuts() scalar, the number of tested parts

### Notes: 
- Scaling modifiers are not applied. The output data is bitwise identical to the original file contents. Expect SI units e.g. Amperes instead of Milliamperes (see "units.txt")
- NaN is used for missing data (skipped tests)
- The testcase generator requires freestdf-libstdf. A small testcase is provided on git, structurally identical to the fullsize testcase
- Endianness conversion is not implemented, if prepared for (reverse byte order in "decode()")
