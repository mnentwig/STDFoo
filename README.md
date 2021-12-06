# STDFoo
a) Converts ATE .stdf.gz to binary float data, one file per TEST_NUM.

b) Imports resulting binary data to Octave _efficiently_ (load-on-demand, designed for xy GB-size datasets or beyond)

* used STDF fields are *PIR* (insertion), *PTR* (individual test data), *PRR* (results/binning). All other records are skipped
* pure C++, needs only a recent compiler e.g. from MinGW, no libraries required except libz. 
* Multithreaded, which gives about 40 % performance boost (note: try -DREFIMPL and compare)
* Fast: A dataset 832 Mb zipped, 30000 simulated DUTs with 2k testitems each is converted in around 12 s on a 2013 PC. More data scales linearly (fixed mem)
* compatible/future-proof: Compiles with -std=(c++11, c++17, c++20, c++23)
* Simple and stupid, both building and running 
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
* testlist: a human-readable / csv style summary with test numbers, names, units and limits

### Octave end:
The quickest 'installation' is to simply copy 'STDFoo.m' into the same directory where 'myOutputDirectory' was created
* o=STDFoo('myOutputDirectory') opens a handle into 'myOutputDirectory'. Hint: available functions below show with tab completion for 'o.' in the Octave GUI
* o.getDataByTestnum(testnum) per DUT. Returns column vector with RESULT(testnum). Vector gives one column per testnum.
* o.getSite() per DUT. Returns used test site.
* o.getHardbin() per DUT. Returns binning information.
* o.getSoftbin() same
* o.getTestnums() per test. Testnumber (sorted)
* o.getUnits() per test. Cellarray of all units (per test order matches getTestnums(
* o.getLowLim() per test. Low limits
* o.getHighLim() per test. High limit
* o.getNDuts() scalar, the number of tested parts

### Notes: 
- Scaling modifiers are not applied. The output data is bitwise identical to the original file contents.
- NaN is used for missing data (skipped tests)
- The testcase generator requires freestdf-libstdf.
- Endianness conversion is not implemented, if prepared for (build separate executable with reversed byte order in "decode()")
