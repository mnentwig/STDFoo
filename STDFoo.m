function o = STDFoo(folder)
% handle-based storage
    persistent db = struct();
    
    % === closure over o and key starts here ===
    % each call to STDFoo(folder) creates a new context
    key = sprintf('_%s', folder);
    o = struct('key', key);

    function data = DUTs_getResultByTestnum(testnum)
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
            end
            data = db.(key).data.(datakey);
        end
    end
    
    
    function r = tests_getTestnums() r = db.(key).testnums; end
    function r = tests_getTestnames() r = db.(key).testnames; end
    function r = tests_getUnits() r = db.(key).units; end
    function r = tests_getLowLim() r = db.(key).lowLim; end
    function r = tests_getHighLim() r = db.(key).highLim; end
    function r = DUTs_getHardbin() r = db.(key).hardbin; end
    function r = DUTs_getSoftbin() r = db.(key).softbin; end
    function r = DUTs_getSite() r = db.(key).site; end
    function r = getNDuts() r = numel(db.(key).site); end
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

    o.DUTs.getResultByTestnum=@DUTs_getResultByTestnum;
    o.tests.getTestnums=@tests_getTestnums;
    o.tests.getTestnames=@tests_getTestnames;
    o.tests.getUnits=@tests_getUnits;
    o.tests.getLowLim=@tests_getLowLim;
    o.tests.getHighLim=@tests_getHighLim;
    o.DUTs.getHardbin=@DUTs.getHardbin;
    o.DUTs.getSoftbin=@DUTs.getSoftbin;
    o.DUTs.getSite=@DUTs.getSite;
    o.getNDuts=@getNDuts;
    o.files.getFiles=@files_getFiles;
    o.files.getDutsPerFile=@files_getDutsPerFile;
    
    % closure over variables specific to 'o' ends here
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
        celldata{end+1} = line;  
    end
    fclose(h);
end
