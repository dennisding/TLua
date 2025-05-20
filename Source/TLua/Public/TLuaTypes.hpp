
#pragma once

#include <string>
#include <vector>
#include <map>
#include <type_traits>

//#include "TLua.hpp"
#include "TLuaImp.hpp"
// #include "TLuaCall.hpp"
//#include "TLuaCppLua.hpp"
#include "TLuaUStructMgr.hpp"
#include "Lua/lua.hpp"
#include "TLuaTypeInfo.hpp"

namespace TLua
{
	template <typename Type>
	inline Type PopValue(lua_State* state);

	//// auto convert
	//template <typename Type>
	//inline Type AutoConverter(Type value) { return value; }

	//template <size_t Size>
	//inline char* AutoConverter(const char(&input)[Size]) { return input; }

	//inline double AutoConverter(float value) { return 0.0; }

	//inline char* AutoConverter(const char *) { return nullptr; }

	template <typename Type>
	Type GetValue(lua_State* State, int Index)
	{
		return TypeInfo<Type>::FromLua(State, Index);
		//using RealType = decltype(AutoConverter(Type()));
		//return (RealType)TypeInfo<RealType>::GetValue(state, index);
	}

	template <typename Type>
	inline void PushValue(lua_State* State, const Type& Value)
	{
//		using RealType = decltype(AutoConverter(value));
		// TypeInfo<RealType>::PushValue(state, value);
//		TypeInfo<RealType>::PushValue(state, value);
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
		//using RealType = decltype(AutoConverter(Type()));

		//RealType result = TypeInfo<RealType>::GetValue(state, -1);
		//LuaPop(state, 1);
		//return (RealType)result;

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
