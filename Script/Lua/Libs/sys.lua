
if _ENV._sys == nil then
    _sys = {} -- init the system
    _sys.paths = {''}
    _sys.suffix = '.lua'
    _sys.module_suffix = '.lum'
    _sys.modules = {}
    _sys.file_reader = function (name)
        local file = io.open(name)
        if file == nil then
            return nil
        end
        local content = file:read('*all')
        file:close()
        return content
    end
end