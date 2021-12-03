# STDFoo
Converts ATE .stdf.gz to binary float data, one file per TEST_NUM, e.g. to be efficiently loaded into Octave.

* Needs only a recent C++ compiler e.g. MinGW g++, no libraries required except libz 
* Multi-(well, dual-)threaded, gives about 40 % performance boost over the single-threaded reference implementation (use -DREFIMPL) 
* Fast: A dataset 832 Mb zipped, 30000 simulated DUTs with 2k testitems each is converted in around 12 s on a 2014 PC.

Command line arguments: output folder (will be created), .stdf.gz file
