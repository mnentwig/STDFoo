function exampleAndSelftest()
  % convert selftest sample file into 'out' folder
  system('STDFoo.exe out testcaseSmall.stdf.gz');

  % open handle
  o = STDFoo('out');

  n = o.getnDUTs();
  
  data11 = o.DUTs.getResultByTestnum(11);
  assert(size(data11) == [n, 1]);
  o.DUTs.uncacheResultByTestnum(11); % use only if cache needs too much memory
  
  data12_13_14 = o.DUTs.getResultByTestnum([12, 13, 14]);
  o.DUTs.uncacheResultByTestnum([12, 13, 14]);  % use only if cache needs too much memory
  sbin = o.DUTs.getSoftbin();
  hbin = o.DUTs.getHardbin();
  site = o.DUTs.getSite();
end