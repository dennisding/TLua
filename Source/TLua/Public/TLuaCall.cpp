
#include "TLuaCall.hpp"

namespace TLua
{
	int RegisterFunction(const char* name)
	{
		// prepare the key
		const int IndexKey = 0;
		lua_State* State = GetLuaState();
		lua_pushinteger(State, IndexKey); // 
		lua_gettable(State, LUA_REGISTRYINDEX);
		int Index = 100;
		if (lua_isnil(State, -1)) {
			Index = 100; // index start from 100
		}
		else {
			Index = lua_tointeger(State, -1);
			Index += 1;
			// push back the IndexKey
			lua_pushinteger(State, IndexKey); // key, 
			lua_pushinteger(State, Index);  // value
			lua_settable(State, LUA_REGISTRYINDEX);
		}
		
		lua_pushinteger(State, Index);
		lua_getglobal(State, name);
		lua_settable(State, LUA_REGISTRYINDEX);
		return Index;
	}
}