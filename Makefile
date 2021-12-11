all: STDFoo.exe
# note: C++17 is default in GCC 11

STDFoo.exe: STDFoo.cpp
	g++ -static -o STDFoo.exe -std=c++17 -O3 -DNODEBUG -Wall STDFoo.cpp -lz
	strip STDFoo.exe

STDFoo_noZ.exe: STDFoo.cpp
	g++ -static -o STDFoo_noZ.exe -std=c++17 -O3 -DNODEBUG -DNO_LIBZ -Wall STDFoo.cpp -lz
	strip STDFoo_noZ.exe

testcase.stdf.gz:
	@echo "needs built freestdf-libstdf directory one level up. Note freeStdf_patches.png for necessary modifications"
# -DDONT_HIDE_TESTCASE: build hack... normally file contents are hidden, as the required Eclipse setup is more complex
	gcc -DDONT_HIDE_TESTCASE -o createTestcase.exe  -I../freestdf-libstdf -I../freestdf-libstdf/include testcase/createTestcase.c ../freestdf-libstdf/src/.libs/libstdf.a -lz -lbz2
	@echo "writing STDF file. This may take a while"
	./createTestcase.exe
	@echo "Zipping STDF file. This may take a while"
	gzip testcase.stdf

testcaseSmall.stdf.gz:
	@echo "needs built freestdf-libstdf directory one level up. Note freeStdf_patches.png for necessary modifications"
# -DDONT_HIDE_TESTCASE: build hack... normally file contents are hidden, as the required Eclipse setup is more complex
	gcc -DSMALL_TESTCASE -DDONT_HIDE_TESTCASE -o createTestcaseSmall.exe  -I../freestdf-libstdf -I../freestdf-libstdf/include testcase/createTestcase.c ../freestdf-libstdf/src/.libs/libstdf.a -lz -lbz2
	@echo "writing STDF file. This may take a while"
	./createTestcaseSmall.exe
	@echo "Zipping STDF file. This may take a while"
	gzip testcaseSmall.stdf

tests: STDFoo.exe testcase.stdf.gz
	@echo "testcaseSmall.stdf.gz" > testjobs.txt
	@echo "testcaseSmall.stdf.gz" >> testjobs.txt
	./STDFoo.exe out1 testcase.stdf.gz testjobs.txt
	@echo 'please also run exampleAndSelftest from octave'

compat:
# gcc 11-2 should successfully build with all those standards (default standard: c++17)
# c++11 uses the POSIX "mkdir" variant internally
	g++ -static -o STDFoo.exe -std=c++11 -O3 -DNODEBUG -Wall STDFoo.cpp -lz
	g++ -static -o STDFoo.exe -std=c++17 -O3 -DNODEBUG -Wall STDFoo.cpp -lz
	g++ -static -o STDFoo.exe -std=c++20 -O3 -DNODEBUG -Wall STDFoo.cpp -lz
	g++ -static -o STDFoo.exe -std=c++23 -O3 -DNODEBUG -Wall STDFoo.cpp -lz

clean:
	rm -Rf STDFoo.exe createTestcase.exe out1 out2 testcase.stdf STDFooRefimpl.exe testjobs.txt

# testcase causes too much hassle to rebuild casually
veryclean: clean
	rm -Rf testcase.stdf.gz testcaseSmall.stdf.gz

.PHONY: clean compat
