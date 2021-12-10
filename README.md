# STDFoo
a) Converts ATE .stdf(.gz) to binary float data, one file per TEST_NUM.

b) Imports resulting binary data to Octave _efficiently_.

Intended for very large datasets from multiple files. 

Output data is organized "column-major" (column=testitem/binning for all DUTs) so that any item of interest can be retrieved with near-optimal speed (=loading a binary file that contains only the test item for all DUTs).

* used STDF fields are *PIR* (insertion), *PTR* (individual test data), *PRR* (results/binning). All other records are skipped
* pure C++, needs only a recent compiler e.g. from MinGW, no libraries required except libz. 
* Fast: Essentially as fast as uncompressing the input file (uncompressing is the bottleneck, other work is multithreaded).
* Quick: Startup overhead is minimal (native executable), equally well suited for a large number of small jobs or a few big ones. 
* Scalable: processing time scales linearly with file size, constant small memory footprint
* compatible/future-proof: Compiles with -std=(c++11, c++17, c++20, c++23)
* "Simple and stupid, robust and reliable" mindset for usage and build process 
_Time will tell whether this has been accomplished_
* Use of Octave end is optional.

### Command line arguments: 
```
STDfoo.exe myOutDirectory myInputfile1.stdf.gz myInputfile2.stdf.gz myInputfile3.stdf.gz ... 
```	
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
The quickest 'installation' is to simply copy 'STDFoo.m' into the same directory where 'myOutputDirectory' was created. Then run Octave from there.
* `o=STDFoo('myOutputDirectory')` opens a handle into 'myOutputDirectory'. 

Available functions show on the command line with tab completion for `o.`.

* `o.DUTs. ...`: Methods return per-DUT data, in the order of PRR records in the STDF file. Note, calling function fields requires round brackets.
* `o.DUTs.getDataByTestnum(testnum)`: Column vector with RESULT(testnum). Giving a vector for `testnum` returns one column per testnum. File contents are cached (subsequent calls for same testnum are faster).
* `o.DUTs.getSite()` Returns used test site.
* `o.DUTs.getHardbin()` Returns final hardbin
* `o.DUTs.getSoftbin()` Returns final softbin

* `o.tests. ...`: Methods return per-test data, sorted by ascending test numbers
* `o.tests.getTestnums()` Testnumber
* `o.tests.getUnits()` Cellarray of all units, matching order in above testnumber. Note: STDF strips scaling factors. E.g. Nano-, Micro-, Milliamperes will all report as "A" with results in Amperes.
* `o.tests.getLowLim()` Low limit of each test (taken from first PTR record where it appeared). Above comment on unscaled / SI units applies.
* `o.tests.getHighLim()` High limit of each test.  Above comment on unscaled / SI units applies.

* `o.files. ...`: Methods return per-file data, in the order of command line arguments given to `STDFoo.exe`.
* `o.files.getFiles()` gets filenames
* `o.files.getDutsPerFile()` DUT count per file
* `o.files.getMaskByFileindex(fileindex)` returns a logical mask to operate on `o.DUTS. ...` data for the given file only.

* `o.getnDUTs()` Total count of tested parts (equals lenght of any `o.DUTs. ...` result)

### Octave examples
```
o=STDFoo('myOutDirectory');
```
Opens a 'handle' o to the output directory of STDFoo.exe. Hint, use tab completion on `o.` to get a list of sections (DUTs, files, ...) and possible commands
```
data = o.DUTs.getResultByTestnum(2345);     % retrieve DUT data for test number 2345
dutNum = 1 : numel(data);                   % x vector for plot
figure(); plot(dutNum, data, 'xk'); hold on;% plot time series of all DUTs as black 'x' (missing entries => NaN => omitted)
sbin = o.DUTs.getSoftbin();                 % get softbin result
mask = sbin == 1234;                        % set up a logical mask that isolates softbin 1234
plot(dutNum(mask), data(mask), 'xk');       % re-plot DUTs that went into softbin 1234 with a red '+'
yield_perc = 100*sum(sbin==1)/numel(sbin)   % calculates yield (assuming soft bin 1 means 'pass')
```

There is a selftest for all Octave features in `selftestExample.m`;

### Compilation
Note: the build process is OS independent. Unix/Linux users would conventionally rename `STDFoo.exe` to `STDFoo` and copy e.g. to `/usr/local/bin`.
```
make STDFoo.exe
```
or run manually e.g.
```
g++ -static -o STDFoo.exe -std=c++17 -O3 -DNODEBUG -Wall STDFoo.cpp -lz
```
Note, all the switches but '-lz' are optional:
* -static Executable should not rely on DLLs / .so libraries (preference)
* -std c++17 is probably the default already, and a higher standard does no harm. Now if the compiler doesn't support c++17, this gives at least a meaningful error.
* -O3 optimize (if benchmarking, try -O2 or -Os. But the bottleneck is largely libz for .stdf.gz)
* -DNDEBUG: Assertions off for higher speed
* -Wall: Complain much (there should still be zero warnings)
* -lz: Link with zlib for uncompressing .gz format

### Notes: 
- Scaling modifiers are not applied. The output data is bitwise identical to the original file contents. Expect SI units e.g. Amperes instead of Milliamperes (see "units.txt")
- NaN is used for missing data (skipped tests)
- The testcase generator requires freestdf-libstdf. A small testcase is provided on git, structurally identical to the fullsize testcase
- Endianness conversion is not implemented, if prepared for (reverse byte order in "decode()")
- Merging multiple files is one of the main use cases (e.g. working with multiple lots, data from different testers, ...). 
Testitems should be "reasonably" consistent between files, because any DUT writes a NaN-result for any missing testitem. 
If two sources of data are largely non-overlapping in testitem numbering, consider processing them individually into separate output folders.
- Fast "row-wise" data extraction (all data for given DUTs) could be applied on the results using `fseek()` as the record size of the output binary data is fixed.