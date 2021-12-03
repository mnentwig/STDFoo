// from freestdf-libstdf examples folder
// gcc -Wall createTestcase.c -I.. -I../include ../src/.libs/libstdf.a -lz -lbz2
// from eclipse workspace (not src)
// gcc -O3 -Wall createTestcase.c -I../freestdf-libstdf -I../freestdf-libstdf/include ../freestdf-libstdf/src/.libs/libstdf.a -lz -lbz2
#include <config.h>
#include <libstdf.h>
#include <internal/headers.h>

int main(int argc, char *argv[]){
  stdf_file *f = stdf_open_ex("testcase.stdf", STDF_OPTS_WRITE | STDF_OPTS_CREATE, 0664);
  if (!f) fprintf(stderr, "failed to open output file\n");
     
  // === write FAR ===
  stdf_rec_far a = { };
  a.CPU_TYPE = STDF_CPU_TYPE_X86;
  a.STDF_VER = 4;
  stdf_init_header(a.header, STDF_REC_FAR);
  stdf_write_record(f, &a);
     
  char buf[256];
  char buf2[256];
  char empty[256];
  sprintf(empty, "%s", "x");
  empty[0] = (unsigned char) (strlen(empty) - 1);
  int partId = 1;
  int siteMax = 4;
  while (partId < 30000) {
    for (int site = 1; site <= siteMax; ++site){ // === write PIR (insertion) ===
      stdf_rec_pir a = { };
      a.HEAD_NUM = 1;
      a.SITE_NUM = site;
      stdf_init_header(a.header, STDF_REC_PIR);
      stdf_write_record(f, &a);
    }
    
    for (int site = 1; site <= siteMax; ++site) {      
      for (int testNum = 11; testNum < 2000; testNum += 1) {
	// === write test data ===
	stdf_rec_ptr a = { };
	stdf_init_header(a.header, STDF_REC_PTR);
	sprintf(buf, "xThisIsTheTestdescriptionWhichIsRepeatedManyTimesForTest%i", testNum);
	buf[0] = (unsigned char) strlen(buf + 1);
	sprintf(buf2, "xmyUnit%i", testNum);
	buf2[0] = (unsigned char) strlen(buf2 + 1);
	a.HEAD_NUM = 1;
	a.SITE_NUM = site;
	a.RESULT = 40000 + testNum + ((float)(partId+site)/3.0f);
	a.TEST_NUM = testNum;
	a.TEST_TXT = buf;
	a.ALARM_ID = (empty);
	a.C_RESFMT = (empty);
	a.C_LLMFMT = (empty);
	a.C_HLMFMT = (empty);
	a.LO_LIMIT = 1000 + 0.1*testNum;
	a.HI_LIMIT = 2000 + 0.1*testNum;
	a.UNITS = (buf2);
	stdf_write_record(f, &a);
      } // for testNum
    } // for site
    
    for (int site = 1; site <= siteMax; ++site) {
      // === write removal / result (binning) ===
      stdf_rec_prr a = { };
      sprintf(buf, "xmyId%i", partId);
      buf[0] = (unsigned char) strlen(buf);
      a.HEAD_NUM = 1;
      a.SITE_NUM = site;
      a.SOFT_BIN = 123;
      a.HARD_BIN = 456;
      a.PART_ID = (buf);
      a.PART_TXT = empty;
      a.PART_FIX = (stdf_dtc_Bn) empty;
      stdf_init_header(a.header, STDF_REC_PRR);
      stdf_write_record(f, &a);
    } // for site
    partId += siteMax;
  } // while partId
  printf("closing\n");fflush(stdout);
  stdf_close(f);
  printf("closed\n");
  fflush(stdout);
  return EXIT_SUCCESS;
}
