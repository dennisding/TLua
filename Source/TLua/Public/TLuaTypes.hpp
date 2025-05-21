
#pragma once

#include <string>
#include <vector>
#include <map>
#include <type_traits>

#include "TLuaImp.hpp"
#include "TLuaUStructMgr.hpp"
#include "Lua/lua.hpp"
#include "TLuaTypeInfo.hpp"

namespace TLua
{
	template <typename Type>
	inline Type PopValue(lua_State* state);

	template <typename Type>
	Type GetValue(lua_State* State, int Index)
	{
		return TypeInfo<Type>::FromLua(State, Index);
	}

	template <typename Type>
	inline void PushValue(lua_State* State, const Type& Value)
	{
		TypeInfo<Type>::ToLua(State, Value);
	}

	template <typename Type>
	inline void PushValues(lua_State* State, const Type& Arg)
	{
		PushValue(State, Arg);
	}

	template <typename First, typename ...Last>
	inline void PushValues(lua_State* State, const First& Value, const Last& ...Args)
	{
		PushValue(State, Value);
		PushValues(State, Args...);
	}

	template <typename Type>
	inline Type PopValue(lua_State* State)
	{
		Type TmpValue;
		TypeInfo<Type>::FromLua(State, -1, TmpValue);
		LuaPop(State, 1);
		return TmpValue;
	}

	template <typename Type>
	inline Type GetTableByName(lua_State* State, int Index, const char* Name)
	{
		int AbsIndex = LuaAbsIndex(State, Index);
		LuaPushString(State, Name);
		LuaGetTable(State, AbsIndex);
		return PopValue<Type>(State);
	}

	template <typename Type>
	inline void SetTableByName(lua_State* State, int Index, const char* Name, const Type& Value)
	{
		int AbsIndex = LuaAbsIndex(State, Index);
		LuaPushString(State, Name); // name
		PushValue(State, Value);	// name, value
		LuaSetTable(State, AbsIndex);
	}
}
