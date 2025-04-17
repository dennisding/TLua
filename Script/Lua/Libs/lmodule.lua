
require('sys')

function _import(name)
	if _sys.modules[name] ~= nil then
		return _sys.modules
	end

	local file_name = name .. _sys.module_suffix
	local file_path = file_name
	local content = nil
	for _, path in ipairs(_sys.paths) do
		file_path = path .. file_name
		print('open the file', file_path)
		content = _sys.file_reader(file_path)
		if content ~= nil then
			break
		end
	end

	-- load the module
	local module = {}
	setmetatable(module, {__index = _ENV})
	_sys.modules[name] = module
	local loaded = load(content, file_path, 'bt', module)
	loaded()

	return module
end