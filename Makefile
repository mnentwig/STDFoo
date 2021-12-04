all: STDFoo.exe
# note: C++17 is default in GCC 11

STDFoo.exe: STDFoo.cpp
	g++ -o STDFoo.exe -std=c++17 -O3 -DNODEBUG -Wall STDFoo.cpp -lz
	strip STDFoo.exe

STDFooRefimpl.exe: STDFoo.cpp
	g++ -DREFIMPL -o STDFooRefimpl.exe -std=c++17 -O3 -DNODEBUG -Wall STDFoo.cpp -lz

testcase.stdf.gz:
	@echo "needs built freestdf-libstdf directory one level up. Note freeStdf_patches.png for necessary modifications"
# -DDONT_HIDE_TESTCASE: build hack... normally file contents are hidden, as the required Eclipse setup is more complex
	gcc -DDONT_HIDE_TESTCASE -o createTestcase.exe  -I../freestdf-libstdf -I../freestdf-libstdf/include createTestcase.c ../freestdf-libstdf/src/.libs/libstdf.a -lz -lbz2
	@echo "writing STDF file. This may take a while"
	./createTestcase.exe
	@echo "Zipping STDF file. This may take a while"
	gzip testcase.stdf

tests: STDFoo.exe STDFooRefimpl.exe testcase.stdf.gz
	@echo "running release version into out1"
	time ./STDFoo.exe out1 testcase.stdf.gz
	@echo "running reference implementation into out2"
	time ./STDFooRefimpl.exe out2 testcase.stdf.gz
	@echo "both out1, out2 folders should be bitwise identical"
	diff -r out1 out2

clean:
	rm -Rf STDFoo.exe createTestcase.exe out1 out2 testcase.stdf testcase.stdf.gz STDFooRefimpl.exe


.PHONY: clean