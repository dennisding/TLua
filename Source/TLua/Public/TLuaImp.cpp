

#include <cstring>
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

function trace_call(fun, ...)
	local function _handler(msg)
		return debug.traceback(msg)
	end

	local ok, msg = xpcall(fun, _handler, ...)
	if not ok then
		_log.warning(utf8_to_utf16(msg))
	end
end
)";

static const char* LuaCodeSys = R"(
_sys = {}
_sys.root = _text('')
_sys.paths = {_text('')}
_sys.suffix = _text('.lua')
_sys.module_suffix = _text('.lum')
_sys.loaded = {} -- use for require
_sys.modules = {} -- use for reloade
_sys.read_file = _cpp_read_file

-- modify the require
local function _format_args(...)
	local t = {}
	for k, v in ipairs{...} do
		t[k] = towstring(v)
	end
	return table.concat(t, _text('    '))
end

-- redefined in Libs/log.lua
function print(...)
	_cpp_log(1, _format_args(...))
end

-- redefined in Libs/log.lua
function warning(...)
	_cpp_log(2, _format_args(...))
end
-- to utf16 string
function towstring(obj)
	if type(obj) == 'string' then
		return obj
	end

	return _cpp_utf8_to_utf16(tostring(obj))
end

utf8_to_utf16 = _cpp_utf8_to_utf16
utf16_to_utf8 = _cpp_utf16_to_utf8

-- modify the default behavior
function require(name)
	local module = _sys.loaded[name]
	if module ~= nil then
		return module
	end

	local display_name = utf16_to_utf8(name .. _sys.suffix)
	for _, path in ipairs(_sys.paths) do
		local file_name = _sys.root .. path .. name .. _sys.suffix

		local module = dofile(file_name, display_name)
		if module ~= nil then
			_sys.modules[name] = module
		end
	end
end

function dofile(file_name, display_name)
	local content = _cpp_read_file(file_name)
	if content == nil then
		return nil
	end
	local chunk, msg = load(content, display_name, 'bt')
	if not chunk then
		_log.warning(utf8_to_utf16(msg))
	end
	
	local result = chunk() or {}
	return result
end

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

	static void CppLog(int level, const FString& msg)
	{
		if (level == 2) {
			UE_LOG(Lua, Warning, TEXT("%s"), *msg);
		}
		else if (level == 4) {
			UE_LOG(Lua, Error, TEXT("%s"), *msg);
		}
		else {
			UE_LOG(Lua, Display, TEXT("%s"), *msg);
		}
	}

	static int CppLog(lua_State* state)
	{
		int level = GetValue<int>(state, 1);
		FString msg = GetValue<FString>(state, 2);

		// check the level definition in Libs/log.lua
		CppLog(level, msg);

		return 0;
	}

	bool CheckState(int r, lua_State* state)
	{
		if (r == LUA_OK) {
			return true;
		}

		if (!lua_isstring(state, -1)) {
			return false;
		}

		// convert utf8 to utf16
		size_t size = 0;
		UTF8CHAR* buff = (UTF8CHAR*)lua_tolstring(state, -1, &size);
		FUTF8ToTCHAR converter(buff, size);
		CppLog(4, converter.Get());
		// pop the message
		lua_pop(state, 1);
		return false;
	}

	static int CppReadFile(lua_State* state)
	{
		auto path = GetValue<FString>(state, 1);
		TArray<uint8> content;

		if (FFileHelper::LoadFileToArray(content, *path, FILEREAD_Silent)){
			lua_pushlstring(state, (const char *)content.GetData(), content.NumBytes());
		}
		else {
			lua_pushnil(state);
		}
		return 1;
	}

	// _text function in lua
	static int CppUTF8_TO_UTF16(lua_State* state)
	{
		size_t utf8_size = 0;
		UTF8CHAR* buff = (UTF8CHAR *)lua_tolstring(state, 1, &utf8_size);

		FUTF8ToTCHAR converter(buff, utf8_size);
		size_t size = converter.Length() * sizeof(TCHAR);
		//lua_pop(state, 1); // pop the buff
		lua_pushlstring(state, (const char*)converter.Get(), size);
		return 1;
	}

	static int CppUTF16_TO_UTF8(lua_State* state)
	{
		FString name = GetValue<FString>(state, 1);
		FTCHARToUTF8 converter(name);
		lua_pushlstring(state, (const char*)converter.Get(), converter.Length());
		return 1;
	}

	static void LoadPrimaryLuaFile(lua_State* state, const FString& basicFileName, const std::string& displayName)
	{
		TArray<uint8> content;

		if (!FFileHelper::LoadFileToArray(content, *basicFileName, FILEREAD_Silent)) {
			UE_LOG(Lua, Error, TEXT("Error in reading basic file[%s]!"), *basicFileName);
			return;
		}

		// do the primary string
		lua_getglobal(state, "load"); // load
		lua_pushlstring(state, (const char*)content.GetData(), content.NumBytes()); // load chunk
		lua_pushstring(state, displayName.c_str());
		lua_pushstring(state, "bt");
		
		CheckState(lua_pcall(state, 3, 1, 0), state);
		CheckState(lua_pcall(state, 0, 0, 0), state);
	}

	void Init()
	{
		lua_State* state = GetLuaState();
		// register the basic utilities
		// internal use
		lua_register(state, "_cpp_callback", CppCallback); // internal use, don't re register this
		lua_register(state, "_lua_set_cpp_attr", LuaSetCppAttr); // internal use, don't re register this
		lua_register(state, "_cpp_utf8_to_utf16", CppUTF8_TO_UTF16);
		lua_register(state, "_cpp_utf16_to_utf8", CppUTF16_TO_UTF8);

		// lib hook, re register this functions when needed
		lua_register(state, "_cpp_read_file", CppReadFile); // re register this after init when needed
		lua_register(state, "_cpp_log", CppLog); // re register this after init when needed

		FString basicFileName = FPaths::ProjectContentDir() / TEXT("Script/Lua/Libs/basic.lua");
		FString sysFileName = FPaths::ProjectContentDir() / TEXT("Script/Lua/Libs/sys.lua");
		LoadPrimaryLuaFile(state, basicFileName, "basic.lua");
		LoadPrimaryLuaFile(state, sysFileName, "sys.lua");

		//DoString(LuaCode, "basic");			// basic code
		//DoString(LuaCodeSys, "sys");		// _sys module code
//		DoString(LuaCodeObj);		// bind the c++ and lua object
		

	}

	inline lua_State* GetLuaState()
	{
		static lua_State* state = NewLuaState();
		return state;
	}

	void DoFile(const FString &name)
	{
		lua_State* state = GetLuaState();

		FTCHARToUTF8 converter(FPaths::GetCleanFilename(name));
		// call the dofile in lua
		lua_getglobal(state, "trace_call");
		lua_getglobal(state, "_lua_dofile");
		PushValue(state, name);
		lua_pushlstring(state, converter.Get(), converter.Length());

		CheckState(lua_pcall(state, 3, 0, 0), state);
	}

	void DoString(const char* buffer, const char* name)
	{
		lua_State* state = GetLuaState();
		size_t size = strlen(buffer);
		
		if (CheckState(luaL_loadbufferx(state, buffer, size, name, "bt"), state))
		{
			LuaCall(state, 0, 0);
		}
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

