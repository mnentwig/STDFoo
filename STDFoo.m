function o = STDFoo(folder)
  % handle-based storage
  persistent db = struct();

  % === closure over o and key starts here ===
  % each call to STDFoo(folder) creates a new context
  o = struct();
  key = sprintf('_%s', folder);
  
  function data = getDataByTestnum(testnum)
    assert(nargin == 1, 'need exactly one argument, which may be a vector');
    if numel(testnum) > 1
      % multiple testnums: return one column per testnum. 
      % preallocate data
      data = nan(getNDuts(), numel(testnum));
      for ix = 1 : numel(testnum)
        data(:, ix) = getDataByTestnum(testnum(ix));
      end
    else
      % single testnum
      datakey = sprintf('d%i', testnum);
      if ~isfield(db.(key).data, datakey)
        db.(key).data.(datakey) = readBinary(folder, sprintf('%i.float', testnum), 'float');
      endif
      data = db.(key).data.(datakey);
    end
  end
  function r = getTestnums() r = db.(key).testnums; end
  function r = getTestnames() r = db.(key).testnames; end
  function r = getLowLim() r = db.(key).lowLim; end
  function r = getHighLim() r = db.(key).highLim; end
  function r = getSite() r = db.(key).site; end
  function r = getNDuts() r = numel(db.(key).site); end
  
  db.(key) = struct(); % clean out existing data
  db.(key).folder = folder;
  db.(key).data = struct();
  db.(key).testnums = readBinary(folder, 'testnums.uint32', 'uint32');
  db.(key).testnames = readString(folder, 'testnames.txt');
  db.(key).units = readString(folder, 'units.txt');
  db.(key).lowLim = readBinary(folder, 'lowLim.float', 'single');
  db.(key).highLim = readBinary(folder, 'highLim.float', 'single');
  db.(key).hardbin = readBinary(folder, 'hardbin.uint16', 'uint16');
  db.(key).softbin = readBinary(folder, 'softbin.uint16', 'uint16');
  db.(key).site = readBinary(folder, 'site.uint8', 'uint8');
  
  o.getDataByTestnum = @getDataByTestnum;
  o.getTestnums=@getTestnums;
  o.getUnits=@getUnits;
  o.getLowLim=@getLowLim;
  o.getHighLim=@getHighLim;
  o.getHardbin=@getHardbin;
  o.getSoftbin=@getSoftbin;
  o.getSite=@getSite;
  o.getNDuts=@getNDuts;
  
% closure over variables specific to 'o' ends here
end

function data = readBinary(folder, fname, bintype)
  fname = [folder, '/', fname];
  h = fopen(fname, 'rb');
  if (h < 0)
    error('failed to open "%s" with type "%s"', fname, bintype);
  endif
  data = fread(h, bintype);
  fclose(h);
end

function celldata = readString(folder, fname)
  celldata = {};
  fname = [folder, '/', fname];
  h = fopen(fname, 'rb');
  while (true)
    line = fgetl(h);
    if (line == -1) 
      break; 
    end
    celldata{end+1} = line;  
  end
  fclose(h);
end