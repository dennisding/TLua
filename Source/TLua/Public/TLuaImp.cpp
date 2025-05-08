

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

		RegisterCppLua();
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

