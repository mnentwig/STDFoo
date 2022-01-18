function exampleAndSelftest()
  % the main purpose of this file is to exercise all functions.

  % convert selftest sample file twice into 'out' folder
  system('STDFoo.exe out testcaseSmall.stdf.gz testcaseSmall.stdf.gz');

  % open handle
  o = STDFoo('out');

  n = o.getnDUTs();
  
  data11 = o.DUTs.getResultByTestnum(11);
  assert(size(data11) == [n, 1]);
  o.DUTs.uncacheResultByTestnum(11); % use only if cache needs too much memory
  
  data12_13_14 = o.DUTs.getResultByTestnum([12, 13, 14]);
  assert(size(data12_13_14) == [n, 3]);
  o.DUTs.uncacheResultByTestnum([12, 13, 14]);  % use only if cache needs too much memory

  sbin = o.DUTs.getSoftbin();
  assert(size(sbin) == [n, 1]);
  hbin = o.DUTs.getHardbin();
  assert(size(hbin) == [n, 1]);
  site = o.DUTs.getSite();
  assert(size(site) == [n, 1]);

  testnums = o.tests.getTestnums();
  nTests = numel(testnums);
  assert(size(testnums) == [nTests, 1]);

  testnames = o.tests.getTestnames();
  assert(size(testnames) == [nTests, 1]);

  tn = o.tests.getTestname(12);
  assert(tn == 'ThisIsTheTestdescriptionWhichIsRepeatedManyTimesForTest12');
  
  units = o.tests.getUnits();
  assert(size(units) == [nTests, 1]);

  lowLim = o.tests.getLowLim();
  assert(size(lowLim) == [nTests, 1]);

  highLim = o.tests.getHighLim();
  assert(size(highLim) == [nTests, 1]);

  files = o.files.getFiles();
  assert(size(files) == [2, 1]);
  
  dutsPerFile = o.files.getDutsPerFile();
  assert(size(dutsPerFile) == [2, 1]);
   
  maskFile1 = o.files.getMaskByFileindex(1); % mask for file1 duts only
  assert(size(maskFile1) == [n, 1]);  

  maskFile2 = o.files.getMaskByFileindex(2); % mask for file2 duts only
  assert(size(maskFile2) == [n, 1]);  
  
  assert(sum(maskFile1 | maskFile2) == n); % both masks must cover every dut
  assert(sum(maskFile1 & maskFile2) == 0);  % both masks may have no duts in common
  
  fileindex = o.files.getFileindex();
  assert(sum(fileindex == 1) == sum(maskFile1));
  assert(sum(fileindex == 2) == sum(maskFile2));
  
  disp('all tests passed');
 end