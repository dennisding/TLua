#pragma once

#include "Lua/lua.hpp"

#define TLUA_TRACE_CALL_NAME "trace_call"

namespace TLua
{
	TLua_API void Init();
	TLua_API void DoFile(const FString& name);
	TLua_API void DoString(const char* buff, const char* name = nullptr);
	TLua_API lua_State* GetLuaState();
	TLua_API bool CheckState(int r, lua_State* state);
	TLua_API void RegisterCallbackImp(const char* name, void* processor, void* callback);
	TLua_API void LuaGetGlobal(lua_State* state, const char* name);
	TLua_API void LuaCall(lua_State* state, int ArgNum, int ReturnNum = 0);
	TLua_API void LuaPCall(lua_State* State, int ArgNum, int ReturnNum = 0);
	TLua_API int LuaGetTop(lua_State* state);
	TLua_API void LuaError(lua_State* state, const char* msg);
	TLua_API void LuaCheckStack(lua_State* State, int Num);
	TLua_API void LuaSetMetatable(lua_State* State, int Index);

	TLua_API void* LuaGetUserData(lua_State* state, int index);
	TLua_API int LuaGetInteger(lua_State* state, int index);
	TLua_API double LuaGetNumber(lua_State* state, int index);
	TLua_API const char* LuaGetString(lua_State* state, int index);
	TLua_API const char* LuaGetLString(lua_State* state, int index, size_t& size);
	TLua_API bool LuaGetBool(lua_State* State, int Index);

	// push value
	TLua_API void LuaPushInteger(lua_State* State, int Value);
	TLua_API void LuaPushBool(lua_State* State, bool Value);
	TLua_API void LuaPushNumber(lua_State* state, double number);
	TLua_API void LuaPushString(lua_State* state, const char* str);
	TLua_API void LuaPushLString(lua_State* state, const char* buff, int size);
	TLua_API void LuaPushUserData(lua_State* state, void* user_data);
	TLua_API void LuaPushNil(lua_State* state);

	TLua_API void LuaNewTable(lua_State* state);
	TLua_API void LuaSetTable(lua_State* state, int index);
	TLua_API void LuaGetTable(lua_State* state, int index);
	TLua_API void LuaLen(lua_State* state, int index);
	TLua_API int LuaNext(lua_State* state, int index);

	TLua_API void LuaPop(lua_State* state, int num);
	TLua_API int LuaAbsIndex(lua_State* state, int index);

	TLua_API void LuaPushCppType(lua_State* State, int CppType);

	TLua_API bool LuaIsTable(lua_State* State, int Index);
	TLua_API void LuaGetField(lua_State* State, int Index, const char* Name);
}
