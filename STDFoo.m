function o = STDFoo(folder)
	  % handle-based storage
    persistent db = struct();
    
    % === closure (effectively) over "key", "folder", "o" starts here ===
	  % see Matlab "Using Handles to Store Function Parameters"
	  % "When you create a function handle for a nested function, that handle stores not only the name of the function, but also the values of externally scoped variables."
    % Effectively, each call to STDFoo(folder) creates a new context
    key = sprintf('_%s', folder);
    o = struct('key', key);
        
	  % note: a cleaner way would be the function(db, o, ...) wrapper approach but this is more compact and slightly faster => keep
    function r = tests_getTestnums() r = db.(key).testnums; end
    function r = tests_getTestnames() r = db.(key).testnames; end
    function r = tests_getUnits() r = db.(key).units; end
    function r = tests_getLowLim() r = db.(key).lowLim; end
    function r = tests_getHighLim() r = db.(key).highLim; end
    function r = DUTs_getHardbin() r = db.(key).hardbin; end
    function r = DUTs_getSoftbin() r = db.(key).softbin; end
    function r = DUTs_getSite() r = db.(key).site; end
    function r = files_getFiles() r = db.(key).files; end
    function r = files_getDutsPerFile() r = db.(key).dutsPerFile; end

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
    db.(key).files = readString(folder, 'filenames.txt');
    db.(key).dutsPerFile = readBinary(folder, 'dutsPerFile.uint32', 'uint32');

    o.DUTs.getResultByTestnum=@(varargin)DUTs_getResultByTestnum(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
    o.DUTs.uncacheResultByTestnum=@(varargin)DUTs_uncacheResultByTestnum(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
    o.tests.getTestnums=@tests_getTestnums;
    o.tests.getTestnames=@tests_getTestnames;
    o.tests.getUnits=@tests_getUnits;
    o.tests.getLowLim=@tests_getLowLim;
    o.tests.getHighLim=@tests_getHighLim;
    o.DUTs.getHardbin=@DUTs_getHardbin;
    o.DUTs.getSoftbin=@DUTs_getSoftbin;
    o.DUTs.getSite=@DUTs_getSite;
    o.getnDUTs=@(varargin)getnDUTs(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
    o.files.getFiles=@files_getFiles;
    o.files.getDutsPerFile=@files_getDutsPerFile;
  	o.files.getMaskByFileindex = @(varargin)files_getMaskByFileindex(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
  	o.files.getFileindex = @(varargin)files_getFileindex(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
end

function r = getnDUTs(db, o) %db, o for object
	assert(nargin == 2+0, 'expecting zero args');
	key = o.key;
	r = numel(db.(key).site); 
end

% note: mask needs 1 bit / DUT
function r = files_getMaskByFileindex(db, o, fileindex) %db, o for object
	assert(nargin == 2 + 1, 'need one input argument fileindex');
	assert(numel(fileindex)==1, 'fileindex must be scalar');
	key = o.key;
	lastIndexInFile = cumsum(db.(key).dutsPerFile);
	firstIndexInFile = [1; lastIndexInFile(1:end-1)+1];
	r = false(getnDUTs(db, o), 1);
	r(firstIndexInFile(fileindex):lastIndexInFile(fileindex)) = true;
end

% note: index needs 64 bit / DUT (double)
function r = files_getFileindex(db, o, fileindex) %db, o for object
	assert(nargin == 2, 'expecting zero args');
	key = o.key;
	lastIndexInFile = cumsum(db.(key).dutsPerFile);
	firstIndexInFile = [1; lastIndexInFile(1:end-1)+1];
	r = zeros(getnDUTs(db, o), 1);
  for fileindex  = 1 : numel(lastIndexInFile)
    r(firstIndexInFile(fileindex):lastIndexInFile(fileindex)) = fileindex;
  end
end

function data = DUTs_getResultByTestnum(db, o, testnum) %db, o for object
	assert(nargin == 2+1, 'need exactly one argument, which may be a vector');
	key = o.key;
	if numel(testnum) > 1
		% multiple testnums: return one column per testnum. 
		% preallocate data
		data = nan(getnDUTs(db, o), numel(testnum));
		for ix = 1 : numel(testnum)
			data(:, ix) = DUTs_getResultByTestnum(db, o, testnum(ix));
		end
	else
		% single testnum
		datakey = sprintf('d%i', testnum);
		if ~isfield(db.(key).data, datakey)
			folder = db.(key).folder;
			db.(key).data.(datakey) = readBinary(folder, sprintf('%i.float', testnum), 'float');
		end
		data = db.(key).data.(datakey);
	end
end

function data = DUTs_uncacheResultByTestnum(db, o, testnum) %db, o for object
	assert(nargin == 2+1, 'need exactly one argument, which may be a vector');
	key = o.key;
	if numel(testnum) > 1
		% multiple testnums: return one column per testnum. 
		% preallocate data
		for ix = 1 : numel(testnum)
			DUTs_uncacheResultByTestnum(db, o, testnum(ix));
		end
	else
		% single testnum
		datakey = sprintf('d%i', testnum);
		if isfield(db.(key).data, datakey)
			rmfield(db.(key).data, datakey);
		end
	end
end

function data = readBinary(folder, fname, bintype)
    fname = [folder, '/', fname];
    h = fopen(fname, 'rb');
    if (h < 0)
        error('failed to open "%s" with type "%s"', fname, bintype);
    end
    data = fread(h, bintype);
    fclose(h);
end
    
function celldata = readString(folder, fname)
    celldata = {};
    fname = [folder, '/', fname];
    h = fopen(fname, 'rb');
    if (h < 0)
        error('failed to open "%s"', fname);
    end
    while (true)
        line = fgetl(h);
        if (line == -1) 
            break; 
        end
        celldata{end+1, 1} = line;  
    end
    fclose(h);
end
