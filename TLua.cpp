
#include "TLua.hpp"

#include "lua.hpp"

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

namespace TLua
{
	void Init()
	{
		lua_State* state = GetLuaState();
		lua_register(state, "_cpp_callback", CppCallback);
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
}