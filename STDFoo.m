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
    function r = tests_getTestnums(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).testnums; 
        if (nargin > 0) r = r(index); end 
    end
    function r = tests_getTestnames(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).testnames; 
        if (nargin > 0) r = r(index); end 
    end
    function r = tests_getTestname(testnum)
        assert(numel(testnum) == 1, 'need scalar testnum argument');
        mask = db.(key).testnums == testnum;
        assert(sum(mask) > 0, 'test with testnum not found');
        assert(sum(mask) == 1, 'more than one test with testnum (invalid data!)');
        r = db.(key).testnames{mask}; 
    end
    
    function r = tests_getUnits(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).units; 
        if (nargin > 0) r = r(index); end 
    end
    function r = tests_getLowLim(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).lowLim; 
        if (nargin > 0) r = r(index); end
    end
    function r = tests_getHighLim(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).highLim; 
        if (nargin > 0) r = r(index); end 
    end
    function r = DUTs_getHardbin(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).hardbin; 
        if (nargin > 0) r = r(index); end 
    end
    function r = DUTs_getSoftbin(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).softbin; 
        if (nargin > 0) r = r(index); end
    end
    function r = DUTs_getSite(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).site; 
        if (nargin > 0) r = r(index); end
    end
    function r = DUTs_getPartId(index)
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        if (~isfield(db.(key), 'partId'))
            db.(key).partId = readString(folder, 'PART_ID.txt');
        end

        % remove final line ('terminator' vs 'separator')
        nDuts = numel(db.(key).site);
        if numel(db.(key).partId) == nDuts + 1
            db.(key).partId(end) = [];
        end
        
        r = db.(key).partId; 
        if (nargin > 0) r = r(index); end 
    end
    function r = DUTs_getPartTxt(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        if (~isfield(db.(key), 'partTxt'))
            db.(key).partTxt = readString(folder, 'PART_TXT.txt');
        end

        % remove final line ('terminator' vs 'separator')
        nDuts = numel(db.(key).site);
        if numel(db.(key).partTxt) == nDuts + 1
            db.(key).partTxt(end) = [];
        end

        r = db.(key).partTxt; 
        if (nargin > 0) r = r(index); end
    end
    function r = files_getFiles(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).files; 
        if (nargin > 0) r = r(index); end
    end
    function r = files_getDutsPerFile(index) 
        assert((nargin >= 0) && (nargin <= 1), 'expecting 0 or 1 args');
        r = db.(key).dutsPerFile; 
        if (nargin > 0) r = r(index); end
    end

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
    o.tests.getTestname=@tests_getTestname;
    o.tests.getTestnames=@tests_getTestnames;
    o.tests.getUnits=@tests_getUnits;
    o.tests.getLowLim=@tests_getLowLim;
    o.tests.getHighLim=@tests_getHighLim;
    o.DUTs.getHardbin=@DUTs_getHardbin;
    o.DUTs.getSoftbin=@DUTs_getSoftbin;
    o.DUTs.getSite=@DUTs_getSite;
    o.DUTs.getPartId=@DUTs_getPartId;
    o.DUTs.getPartTxt=@DUTs_getPartTxt;
    o.DUTs.getFileindex = @(varargin)DUTs_getFileindex(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
    o.DUTs.getIndexInFile = @(varargin)DUTs_getIndexInFile(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
    o.getnDUTs=@(varargin)getnDUTs(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
    o.files.getFiles=@files_getFiles;
    o.files.getDutsPerFile=@files_getDutsPerFile;
    o.files.getMaskByFileindex = @(varargin)files_getMaskByFileindex(db, o, varargin{:}); % boilerplate wrapper prepending db, o args
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

% note: getMaskByFileindex is usually much more efficient (1 bit logical indexing vs 64-bit double)
function r = DUTs_getFileindex(db, o, index) %db, o for object
    assert((nargin >= 2) && (nargin <= 3), 'expecting 0 or 1 args');
    key = o.key;
    lastIndexInFile = cumsum(db.(key).dutsPerFile);
    firstIndexInFile = [1; lastIndexInFile(1:end-1)+1];
    r = zeros(getnDUTs(db, o), 1);
    for fileindex  = 1 : numel(lastIndexInFile)
        r(firstIndexInFile(fileindex):lastIndexInFile(fileindex)) = fileindex;
    end
    if (nargin > 2) r = r(index); end 
end

function r = DUTs_getIndexInFile(db, o) %db, o for object
    assert(nargin == 2, 'expecting 0 args');
    key = o.key;
    lastIndexInFile = cumsum(db.(key).dutsPerFile);
    firstIndexInFile = [1; lastIndexInFile(1:end-1)+1];
    nDutsInFile = lastIndexInFile - firstIndexInFile + 1;
    r = zeros(getnDUTs(db, o), 1);
    for fileindex  = 1 : numel(lastIndexInFile)
        r(firstIndexInFile(fileindex):lastIndexInFile(fileindex)) = 1:nDutsInFile(fileindex);
    end
end

function data = DUTs_getResultByTestnum(db, o, testnum) %db, o for object
    assert(nargin == 2+1, 'need exactly one argument (testnum), which may be vector or scalar');
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
            db.(key).data = rmfield(db.(key).data, datakey);
        end
    end
end

% reads binary file into numerical vector 
function data = readBinary(folder, fname, bintype)
    fname = [folder, '/', fname];
    h = fopen(fname, 'rb');
    if (h < 0)
        error('failed to open "%s" with type "%s"', fname, bintype);
    end
    data = fread(h, bintype);
    fclose(h);
end
    
% reads newline-separated file into cell array of strings
function celldata = readString(folder, fname)
    fname = [folder, '/', fname];
    h = fopen(fname, 'rb');
    if (h < 0)
        error('failed to open "%s"', fname);
    end
    tmp = fread(h, [1, Inf], 'char=>char');
    fclose(h);
    celldata = strsplit(tmp, char(10), 'collapsedelimiters', false).';
    assert(isempty(celldata{end}), 'expecting newline termination after last line');
    celldata(end) = [];
end
