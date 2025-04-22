

#include <fstream>
#include <iostream>

#include "TLua.hpp"

static inline lua_State* NewLuaState()
{
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);
	return state;
}

static inline int CppCallback(lua_State* state)
{
	lua_CFunction callback = (lua_CFunction)lua_touserdata(state, 1);
	return callback(state);
}

static inline int LuaSetCppAttr(lua_State* state)
{
	lua_CFunction callback = (lua_CFunction)lua_touserdata(state, 1);
	return callback(state);
}

static const char *LuaCode = R"(
-- table for c++ function callback
_cpp = {}
function _register_callback(name, processor, cfun)
	_cpp = _cpp or {}
	local function _imp(...)
		_cpp_callback(processor, cfun, ...)
	end
	_cpp[name] = _imp
end
)";

static const char* LuaCodeSys = R"(
_sys = _sys or {}
_sys.paths = {'', 'Script/Lua/', 'Script/Lua/Entities/', 'Script/Lua/Libs/', 'Script/Lua/Common/'}
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

-- modify the require 
local function _load_package(name)
	for _, path in ipairs(_sys.paths) do
		local file_name = path .. name .. _sys.suffix
		local content = _sys.file_reader(file_name)
		if content ~= nil then
			return load(content, file_name)
		end
	end
end
table.insert(package.searchers, 1, _load_package)
)";

const char* LuaCodeObj = R"(
-- replace this in your cpp code
-- table for c++ object
local _cobjs = _ENV._cobjs or {}
-- table for vtable
local _vtables = _ENV._vtables or {}

function _cpp_bind_obj(obj, class_name)
	print('bind obj', obj, class_name)
	local instance = _ENV[class_name]()
	instance._cobj = obj
	instance._vtable = _vtables[class_name]

	_cobjs[obj] = instance
end

function _cpp_unbind_obj(obj)
	print('unbind obj', obj)

	assert(_cobjs[obj] ~= nil)
	_cobjs[obj] = nil
end

function _cpp_obj_call(obj, name, ...)
	local instance = _cobjs[obj]
	return instance[name](instance, ...)
end

function _cpp_obj_getattr(obj, name)
	return _cobjs[obj][name]
end

function _cpp_obj_setattr(obj, name, value)
	_cobjs[obj][name] = value
end

-- vtable implement
function _cpp_new_vtable(name)
	local vt = {}
	vt._type_name = name
	_vtables[name] = vt
end

function _cpp_vtable_add_attr(class_name, attr_name, get_processor, getter, set_processor, setter)
	local function _getter(self)
		return _cpp_callback(get_processor, getter, class_name, self, attr_name)
	end

	local function _setter(self, value)
		_cpp_callback(set_processor, setter, class_name, self, attr_name, value)
	end

	_vtables[class_name][attr_name] = { _getter, _setter}
end

function _cpp_vtable_add_method(class_name, method_name, processor, callback)
	print('vtable add method', class_name, method_name, processor, callback)
	local function _method(self, ...)
		_cpp_callback(processor, callback, class_name, self, method_name, ...)
	end
	_vtables[class_name][method_name] = _method
end
)";

namespace TLua
{
	bool CheckState(int r, lua_State* state)
	{
		if (r == LUA_OK) {
			return true;
		}

		int index = lua_gettop(state);
		lua_getglobal(state, "_cpp_log");
		lua_pushinteger(state, 1);
		lua_pushvalue(state, index);
		lua_pcall(state, 2, 0, 0);
		return false;
	}

	static int CppOpenFile(lua_State* state)
	{
		std::string filename = GetValue<std::string>(state, -1);

		std::ifstream file(filename, std::ifstream::binary);
		if (file.fail()) {
			return 0;
		}
		// get the length of file
		file.seekg(0, file.end);
		size_t size = file.tellg();
		file.seekg(0, file.beg);

		char* buffer = new char[size + 1];
		buffer[size] = 0;

		file.read(buffer, size);

		// read the file as string
		lua_pushlstring(state, buffer, size);

		delete []buffer;
		file.close();

		return 1;
	}

	static int CppLog(lua_State* state)
	{
		int logLevel = GetValue<int>(state, 1);
		std::string msg = GetValue<std::string>(state, 2);

		std::cout << "Log:" << logLevel << ":" << msg << std::endl;

		return 0;
	}

	void Init()
	{
		lua_State* state = GetLuaState();
		// register the basic utilities
		// internal use
		lua_register(state, "_cpp_callback", CppCallback); // internal use, don't re register this
		lua_register(state, "_lua_set_cpp_attr", LuaSetCppAttr); // internal use, don't re register this

		// lib hook, re register this functions when needed
		lua_register(state, "_cpp_openfile", CppOpenFile); // re register this after init when needed
		lua_register(state, "_cpp_log", CppLog); // re register this after init when needed

		DoString(LuaCode);			// basic code
		DoString(LuaCodeSys);		// _sys module code
		DoString(LuaCodeObj);		// bind the c++ and lua object
	}

	inline lua_State* GetLuaState()
	{
		static lua_State* state = NewLuaState();
		return state;
	}

	void DoFile(const char* file_name)
	{
		lua_State* state = GetLuaState();
		CheckState(luaL_dofile(state, file_name), state);
	}

	void DoString(const char* buffer)
	{
		lua_State* state = GetLuaState();
		CheckState(luaL_loadstring(state, buffer), state);
		CheckState(lua_pcall(state, 0, 0, 0), state);
	}

	void RegisterCallback(const char* name, void* processor, void* callback)
	{
		lua_State* state = GetLuaState();

		lua_getglobal(state, "_register_callback");
		lua_pushstring(state, name);
		lua_pushlightuserdata(state, processor);
		lua_pushlightuserdata(state, callback);

		lua_call(state, 3, 0);
	}

	void LuaGetGlobal(lua_State* state, const char* name)
	{
		lua_getglobal(state, name);
	}

	void LuaCall(lua_State* state, int arg_num)
	{
		CheckState(lua_pcall(state, arg_num, 0, 0), state);
	}
}

