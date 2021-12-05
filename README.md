# STDFoo
Converts ATE .stdf.gz to binary float data, one file per TEST_NUM. Example use case: import to Octave / Matlab.

* pure C++, needs only a recent compiler e.g. from MinGW, no libraries required except libz 
* Multithreaded, which gives about 40 % performance boost (note: try -DREFIMPL and compare)
* Fast: A dataset 832 Mb zipped, 30000 simulated DUTs with 2k testitems each is converted in around 12 s on a 2013 PC.
* compatible/future-proof: Compiles with -std=(c++11, c++17, c++20, c++23)
* Simple and stupid, both building and running 
* used STDF fields are *PIR* (insertion), *PTR* (individual test data), *PRR* (results/binning). All other records are skipped

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

### Notes: 
- Scaling modifiers are not applied. The output data is bitwise identical to the original file contents.
- NaN is used for missing data (skipped tests)
- The testcase generator requires freestdf-libstdf.
- Endianness conversion is not implemented, if somewhat prepared for (build separate executable for speed with reversed byte order in "stdfWriter::decode()")
