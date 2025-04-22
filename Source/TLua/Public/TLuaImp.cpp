

#include <fstream>
#include <iostream>

#include "TLua.h"
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
_cpp = {}		-- all cpp function register here
_texts = {}		-- utf8 to utf16 cache
function _register_callback(name, processor, cfun)
	_cpp = _cpp or {}
	local function _imp(...)
		return _cpp_callback(processor, cfun, ...)
	end
	_cpp[name] = _imp
end

function _text(str)
	local text = _texts[str]
	if text ~= nil then
		return text
	end

	text = _cpp_utf8_to_utf16(str)
	_texts[str] = text
	return text
end
)";

static const char* LuaCodeSys = R"(
_sys = _sys
_sys.root = _text('')
_sys.paths = {_text('')}
_sys.suffix = _text('.lua')
_sys.module_suffix = _text('.lum')
_sys.modules = {}
_sys.read_file = function (name)
	return _cpp_open_file(name)
end

-- modify the require 
local function _load_package(name)
	for _, path in ipairs(_sys.paths) do
		local file_name = _sys.root .. path .. name .. _sys.suffix
		local content = _cpp_open_file(file_name)
		if content ~= nil then
			return load(content, file_name)
		end
	end
end
table.insert(package.searchers, 1, _load_package)

function _init_sys(root, dirs)
	_sys.root = root
	_sys.paths = dirs
end
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
		//std::string filename = GetValue<std::string>(state, -1);

		//std::ifstream file(filename, std::ifstream::binary);
		//if (file.fail()) {
		//	return 0;
		//}
		//// get the length of file
		//file.seekg(0, file.end);
		//size_t size = file.tellg();
		//file.seekg(0, file.beg);

		//char* buffer = new char[size + 1];
		//buffer[size] = 0;

		//file.read(buffer, size);

		//// read the file as string
		//lua_pushlstring(state, buffer, size);

		//delete []buffer;
		//file.close();
		// FString path = FPaths::ProjectDir() / TEXT("init.lua");
		auto path = GetValue<FString>(state, 1);
		TArray<uint8> content;

		if (FFileHelper::LoadFileToArray(content, *path)){
			// file loaded
		//	lua_pushlstring(state, content, content.Size)
			lua_pushlstring(state, (const char *)content.GetData(), content.NumBytes());
			return 1;
		}
		else {
			return 0;
		}
	}

	static int CppLog(lua_State* state)
	{
		//int logLevel = GetValue<int>(state, 1);
		//std::string msg = GetValue<std::string>(state, 2);

		//std::cout << "Log:" << logLevel << ":" << msg << std::endl;
		int level = GetValue<int>(state, 1);
		const char* msg = GetValue<const char*>(state, 2);

		UE_LOG(Lua, Warning, TEXT("%hs"), msg);

		return 0;
	}

	// _text function in lua
	static int CppText(lua_State* state)
	{
		size_t size = 0;
		UTF8CHAR* buff = (UTF8CHAR *)lua_tolstring(state, 0, &size);

		FUTF8ToTCHAR converter(buff);
		lua_pop(state, 1); // pop the buff
		lua_pushlstring(state, (const char*)converter.Get(), converter.Length());
		return 1;
	}

	void Init()
	{
		lua_State* state = GetLuaState();
		// register the basic utilities
		// internal use
		lua_register(state, "_cpp_callback", CppCallback); // internal use, don't re register this
		lua_register(state, "_lua_set_cpp_attr", LuaSetCppAttr); // internal use, don't re register this
		lua_register(state, "_cpp_utf8_to_utf16", CppText);

		// lib hook, re register this functions when needed
		lua_register(state, "_cpp_open_file", CppOpenFile); // re register this after init when needed
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
		FString path = FPaths::ProjectDir() / file_name;
		TArray<uint8> content;

		if (FFileHelper::LoadFileToArray(content, *path)) {
			// file loaded
			lua_pushlstring(state, (const char*)content.GetData(), content.Num());
			LuaCall(state, 0, 0);
		}
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

	void LuaCall(lua_State* state, int arg_num, int return_num)
	{
		CheckState(lua_pcall(state, arg_num, return_num, 0), state);
	}

	int LuaGetTop(lua_State* state)
	{
		return lua_gettop(state);
	}

	void LuaError(lua_State* state, const char* msg)
	{
		lua_pushstring(state, msg);
		lua_error(state);
	}


	void* LuaGetUserData(lua_State* state, int index)
	{
		return lua_touserdata(state, index);
	}

	int LuaGetInteger(lua_State* state, int index)
	{
		return lua_tointeger(state, index);
	}
	double LuaGetNumber(lua_State* state, int index)
	{
		return lua_tonumber(state, index);
	}
	std::string LuaGetString(lua_State* state, int index)
	{
		return std::string(lua_tostring(state, index));
	}

	const char* LuaGetLString(lua_State* state, int index, size_t &size)
	{
		return lua_tolstring(state, index, &size);
	}

	void LuaNewTable(lua_State* state)
	{
		lua_newtable(state);
	}

	void LuaSetTable(lua_State* state, int index)
	{
		lua_settable(state, index);
	}

	void LuaGetTable(lua_State* state, int index)
	{
		lua_gettable(state, index);
	}

	void LuaLen(lua_State* state, int index)
	{
		lua_len(state, index);
	}

	int LuaNext(lua_State* state, int index)
	{
		return lua_next(state, index);
	}

	void LuaPop(lua_State* state, int num)
	{
		lua_pop(state, num);
	}

	int LuaAbsIndex(lua_State* state, int index)
	{
		return lua_absindex(state, index);
	}

	// push the value
	//void LuaPushInteger(Lua_State* state, int iv)
	//{

	//}

	void LuaPushInteger(lua_State* state, int iv)
	{
		lua_pushinteger(state, iv);
	}

	void LuaPushNumber(lua_State* state, double number)
	{
		lua_pushnumber(state, number);
	}

	void LuaPushString(lua_State* state, const char* str)
	{
		lua_pushstring(state, str);
	}

	void LuaPushLString(lua_State* state, const char* str, int size)
	{
		lua_pushlstring(state, str, size);
	}

	void LuaPushUserData(lua_State* state, void* user_data)
	{
		lua_pushlightuserdata(state, user_data);
	}

	void LuaPushNil(lua_State* state)
	{
		lua_pushnil(state);
	}
}

