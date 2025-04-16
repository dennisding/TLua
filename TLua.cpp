

#include <fstream>
#include <iostream>

#include "lua.hpp"
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

static const char *LuaCode = R"(
function _register_callback(name, processor, cfun)
	print('register a callback in lua', name, processor, cfun)
	_cpp = _cpp or {}
	local function _imp(...)
		_cpp_callback(processor, cfun, ...)
	end
	_cpp[name] = _imp
end
)";

namespace TLua
{
	static int CppOpenFile(lua_State* state)
	{
		std::string filename = GetValue<std::string>(state, -1);

		std::ifstream file(filename, std::ifstream::binary);
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
		lua_register(state, "_cpp_callback", CppCallback);
		lua_register(state, "_cpp_openfile", CppOpenFile); // re register this after init when needed
		lua_register(state, "_cpp_log", CppLog); // re register this after init when needed

		DoString(LuaCode);
	}

	inline lua_State* GetLuaState()
	{
		static lua_State* state = NewLuaState();
		return state;
	}

	void DoFile(const std::string& file_name)
	{
		lua_State* state = GetLuaState();
		luaL_dofile(state, file_name.c_str());
	}

	void DoString(const char* buffer)
	{
		lua_State* state = GetLuaState();
		luaL_loadstring(state, buffer);
		lua_pcall(state, 0, 0, 0);
	}
}