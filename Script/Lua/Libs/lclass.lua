
function _super(type, instance)
	local wraper = {}

	local function _index(t, key)
		local super_method = nil
		while type._super ~= nil do
			super_method = type._super[key]
			if super_method ~= nil then
				break
			end
		end
		local function _method(...)
			if super_method ~= nil then
				return super_method(instance, ...)
			end
		end

		return _method
	end

	local meta = {__index = _index }
	setmetatable(wraper, meta)

	return wraper
end

function _class(name, super)
	local _new_class = {_type_name = name or "<unknown>", _super = super}
	_new_class.__index = _new_class

	local function _call(type, ...)
			local instance = {}
			setmetatable(instance, _new_class)

			local _init = _new_class._init
			if _init ~= nil then
				_init(instance, ...)
			end

			return instance
	end

	setmetatable(_new_class, {__call = _call})

	return _new_class
end
