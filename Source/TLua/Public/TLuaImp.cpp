

#include <cstring>
#include <fstream>
#include <iostream>

#include "TLua.h"
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

#include "TLua.hpp"
#include "TLuaCppLua.hpp"
#include "TLuaTypes.hpp"

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

	static int LuaCppLog(lua_State* state)
	{
		int level = TypeInfo<int>::FromLua(state, 1);
		FString msg = TypeInfo<FString>::FromLua(state, 2);

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
		FString msg(converter.Length(), converter.Get());
		CppLog(4, msg);

		return false;
	}

	static int CppReadFile(lua_State* state)
	{
		auto path = TypeInfo<FString>::FromLua(state, 1);
		TArray<uint8> content;

		if (FFileHelper::LoadFileToArray(content, *path, FILEREAD_Silent)){
			content.Add(0);
			lua_pushlstring(state, (const char *)content.GetData(), content.NumBytes() - 1);
		}
		else {
			lua_pushnil(state);
		}
		return 1;
	}

	// _text function in lua
	static int CppUTF8_TO_UTF16(lua_State* State)
	{
		size_t utf8_size = 0;
		UTF8CHAR* buff = (UTF8CHAR *)lua_tolstring(State, 1, &utf8_size);

		FUTF8ToTCHAR converter(buff, utf8_size);
		size_t size = converter.Length() * sizeof(TCHAR);
		lua_pushlstring(State, (const char*)converter.Get(), size);
		return 1;
	}

	static int CppUTF16_TO_UTF8(lua_State* state)
	{
		FString name = TypeInfo<FString>::FromLua(state, 1);
		FTCHARToUTF8 converter(name);
		lua_pushlstring(state, (const char*)converter.Get(), converter.Length());
		return 1;
	}

	static void LoadPrimaryLuaFile(lua_State* State, const FString& BaseName, const std::string& DisplayName)
	{
		TArray<uint8> Content;

		if (!FFileHelper::LoadFileToArray(Content, *BaseName, FILEREAD_Silent)) {
			UE_LOG(Lua, Error, TEXT("error while reading basic file:[%s]"), *BaseName);
			return;
		}

		Content.Add(0); // null byte
		int RecoverIndex = lua_gettop(State);
		lua_getglobal(State, "load");
		int TopIndex = lua_gettop(State);
		lua_pushlstring(State, (const char*)Content.GetData(), Content.NumBytes() - 1);
		lua_pushstring(State, DisplayName.c_str());
		lua_call(State, 2, LUA_MULTRET);
		if (lua_isnoneornil(State, TopIndex)) {
			FString Name(DisplayName.c_str()); // convert to utf16
			FString Msg(lua_tostring(State, TopIndex + 1));
			UE_LOG(Lua, Error, TEXT("Failed in load file[%s], %s"), *Name, *Msg);
			lua_settop(State, RecoverIndex);
			return;
		}

		int Result = lua_pcall(State, 0, 0, 0);
		if (Result != LUA_OK) {
			// PRINT ERROR MSG
			FString ErrorMsg(lua_tostring(State, TopIndex));
			UE_LOG(Lua, Error, TEXT("Failed in call lua code: %s"), *ErrorMsg);
			lua_settop(State, RecoverIndex);
		}
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
		RegisterCppLua();

		// lib hook, re register this functions when needed
		lua_register(state, "_cpp_read_file", CppReadFile); // re register this after init when needed
		lua_register(state, "_cpp_log", LuaCppLog); // re register this after init when needed

		FString basicFileName = FPaths::ProjectContentDir() / TEXT("Script/Lua/Libs/basic.lua");
		FString sysFileName = FPaths::ProjectContentDir() / TEXT("Script/Lua/Libs/sys.lua");
		LoadPrimaryLuaFile(state, basicFileName, "basic.lua");
		LoadPrimaryLuaFile(state, sysFileName, "sys.lua");
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
		lua_getglobal(state, "trace_call");			// trace_call
		lua_getglobal(state, "_lua_dofile");		// trace_call, _lua_dofile
		PushValue(state, name);						// trace_call, _lua_dofile, name
		lua_pushlstring(state, converter.Get(), converter.Length());

		lua_call(state, 3, 0);
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

	void RegisterCallbackImp(const char* name, void* processor, void* callback)
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

	void LuaCall(lua_State* State, int ArgNum, int ReturnNum)
	{
		int Result = lua_pcall(State, ArgNum, ReturnNum, 0);
		if (Result != LUA_OK) {
			FString Msg(lua_tostring(State, -1));
			UE_LOG(Lua, Error, TEXT("error in call lua code: %s"), *Msg);
		}
	}

	void LuaPCall(lua_State* State, int ArgNum, int ReturnNum)
	{
		int Result = lua_pcall(State, ArgNum, ReturnNum, 0);
		if (Result != LUA_OK) {
			FString Msg(lua_tostring(State, -1));
			UE_LOG(Lua, Error, TEXT("error in call lua code: %s"), *Msg);
		}
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

	void LuaCheckStack(lua_State* State, int Num)
	{
		lua_checkstack(State, Num);
	}

	void* LuaGetUserData(lua_State* state, int index)
	{
		return lua_touserdata(state, index);
	}

	void* LuaNewUserData(lua_State* State, size_t Size, int uv)
	{
		return lua_newuserdatauv(State, Size, uv);
	}

	int LuaGetInteger(lua_State* state, int index)
	{
		return lua_tointeger(state, index);
	}

	double LuaGetNumber(lua_State* state, int index)
	{
		return lua_tonumber(state, index);
	}

	const char* LuaGetString(lua_State* state, int index)
	{
		return lua_tostring(state, index);
	}

	const char* LuaGetLString(lua_State* state, int index, size_t &size)
	{
		return lua_tolstring(state, index, &size);
	}

	bool LuaGetBool(lua_State* State, int Index)
	{
		return (bool)lua_toboolean(State, Index);
	}

	void LuaNewTable(lua_State* state)
	{
		lua_newtable(state);
	}

	void LuaSetTable(lua_State* state, int index)
	{
		lua_settable(state, index);
	}

	void LuaSetField(lua_State* State, int Index, const char* Key)
	{
		lua_setfield(State, Index, Key);
	}

	void LuaSetI(lua_State* State, int Index, lua_Integer N)
	{
		lua_seti(State, Index, N);
	}

	void LuaGetI(lua_State* State, int Index, lua_Integer N)
	{
		lua_geti(State, Index, N);
	}

	void LuaGetTable(lua_State* state, int index)
	{
		lua_gettable(state, index);
	}

	int LuaGetTableSize(lua_State* State, int Index)
	{
		lua_len(State, Index);
		int length = lua_tointeger(State, -1);
		lua_pop(State, 1);
		return length;
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

	void LuaPushInteger(lua_State* State, int Value)
	{
		lua_pushinteger(State, Value);
	}

	void LuaPushBool(lua_State* State, bool Value)
	{
		lua_pushboolean(State, Value);
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

	void LuaPushCFunction(lua_State* State, lua_CFunction Fun)
	{
		lua_pushcfunction(State, Fun);
	}

	void LuaPushCppType(lua_State* State, int Type)
	{
		lua_pushinteger(State, Type);
		lua_gettable(State, LUA_REGISTRYINDEX);
	}

	void LuaSetMetatable(lua_State* State, int Index)
	{
		lua_setmetatable(State, Index);
	}

	bool LuaIsTable(lua_State* State, int Index)
	{
		return lua_istable(State, Index);
	}

	void LuaGetField(lua_State* State, int Index, const char* Name)
	{
		lua_getfield(State, Index, Name);
	}
}

