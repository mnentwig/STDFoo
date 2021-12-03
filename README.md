# STDFoo
Converts ATE .stdf.gz to binary float data, one file per TEST_NUM, e.g. to be efficiently loaded into Octave.

* Needs only a recent C++ compiler e.g. MinGW g++, no libraries required except libz 
* Multi-(well, dual-)threaded, gives about 40 % performance boost over the single-threaded reference implementation (use -DREFIMPL) 
* Fast: A dataset 832 Mb zipped, 30000 simulated DUTs with 2k testitems each is converted in around 12 s on a 2014 PC.

### Command line arguments: 
STDfoo.exe myOutDirectory myInputfile.stdf.gz
myOutDirectory will be created.

### Results in myOutputDirectory:
* testnums.uint32: all encountered TEST_NUM fields in ascending order
* testnames.txt: newline-separated TEST_DESC strings, one per TEST_NUM
* units.txt: newline-separated UNIT for each TEST_NUM
* lowLim.float: Low limit corresponding to TEST_NUM 
* highLim.float: High limit corresponding to TEST_NUM
* num.float: One file (32-bit single precision binary) with RESULT of all DUTs, in order of ejection (matches order of PRR records in file)
* hardbin.u16: The hardbin for each DUT
* softbin.u16: The softbin for each DUT
* site.u8: The site where each DUT was tested

### Notes: 
- Scaling modifiers are not applied. The output data is bitwise identical to the original file contents.
- NaN is used for missing data (skipped tests)
- The testcase generator requires freestdf-libstdf.
- Endianness conversion is not implemented.
