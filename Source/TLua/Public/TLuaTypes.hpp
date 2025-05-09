
#pragma once

#include <string>
#include <vector>
#include <map>
#include <type_traits>

#include "TLua.hpp"
#include "TLuaImp.hpp"
#include "Lua/lua.hpp"

namespace TLua
{
	template <typename Type>
	inline Type PopValue(lua_State* state);

	template <typename Type>
	struct TypeInfo {};

	template <>
	struct TypeInfo<int>
	{
		inline static int GetValue(lua_State* state, int index)
		{
			return (int)LuaGetInteger(state, index);
		}

		inline static void PushValue(lua_State* state, int value)
		{
			LuaPushInteger(state, value);
		}
	};

	template <>
	struct TypeInfo<double>
	{
		inline static double GetValue(lua_State* state, int index)
		{
			return LuaGetNumber(state, index);
		}

		inline static void PushValue(lua_State* state, double value)
		{
			LuaPushNumber(state, value);
		}
	};

	template <>
	struct TypeInfo<char*>
	{
		inline static const char* GetValue(lua_State* state, int index)
		{
			return LuaGetString(state, index);
		}

		inline static void PushValue(lua_State* state, const char* value)
		{
			LuaPushString(state, value);
		}
	};

	template <>
	struct TypeInfo<std::string>
	{
		inline static std::string GetValue(lua_State* state, int index)
		{
			size_t size = 0;
			const char* buffer = LuaGetLString(state, index, size);
			return std::string(buffer, size);
		}

		inline static void PushValue(lua_State* state, const std::string& value)
		{
			LuaPushLString(state, value.c_str(), value.size());
		}
	};

	template <>
	struct TypeInfo<FString>
	{
		inline static FString GetValue(lua_State* state, int index)
		{
			size_t size = 0;
			const TCHAR* buff = (const TCHAR*)LuaGetLString(state, index, size);

			return FString(size / sizeof(TCHAR), buff);
		}

		inline static void PushValue(lua_State* state, const FString& value)
		{
			LuaPushLString(state, (const char*)(*value), value.NumBytesWithoutNull());
		}
	};

	template <>
	struct TypeInfo<FName>
	{
		inline static FName GetValue(lua_State* State, int Index)
		{
			size_t Size = 0;
			const TCHAR* Buff = (const TCHAR*)LuaGetLString(State, Index, Size);
			FName Name(Size/sizeof(TCHAR), Buff);

			return Name;
		}

		inline static void PushValue(lua_State* State, const FName& Name)
		{
			FString NameString = Name.ToString();
			TypeInfo<FString>::PushValue(State, NameString);
		}
	};

	template <typename Type>
	struct TypeInfo<Type*>
	{
		inline static Type* GetValue(lua_State* state, int index)
		{
			return (Type*)LuaGetUserData(state, index);
		}

		inline static void PushValue(lua_State* state, const Type* ptr)
		{
			LuaPushUserData(state, (void*)ptr);
		}
	};

	template <typename ValueType>
	struct TypeInfo<TArray<ValueType>>
	{
		using Type = TArray<ValueType>;
		inline static Type GetValue(lua_State* state, int index)
		{
			// check this is a table
			int abs_index = LuaAbsIndex(state, index);

			// get table length
			LuaLen(state, abs_index);
			int table_size = PopValue<int>(state);

			Type result;
			result.Reserve(table_size);
			for (int i = 1; i <= table_size; ++i) {
				TypeInfo<int>::PushValue(state, i);
				LuaGetTable(state, abs_index);
				result.Emplace(PopValue<ValueType>(state));
			}
			return result;
		}

		inline static void PushValue(lua_State* state, const Type& values)
		{
			LuaNewTable(state);
			for (int i = 0; i < values.Num(); ++i) {
				TypeInfo<int>::PushValue(state, i + 1);
				TypeInfo<ValueType>::PushValue(state, values[i]);
				LuaSetTable(state, -3);
			}
		}
	};

	template <typename Type>
	inline Type AutoConverter(Type value) { return value; }

	inline double AutoConverter(float value) { return 0.0; }

	template <typename Type>
	auto GetValue(lua_State* state, int index)
	{
		using RealType = decltype(AutoConverter(Type()));
		return (RealType)TypeInfo<RealType>::GetValue(state, index);
	}

	template <typename Type>
	inline void PushValue(lua_State* state, const Type& value)
	{
		using RealType = decltype(AutoConverter(value));
		TypeInfo<RealType>::PushValue(state, value);
	}

	template <typename Type>
	inline void PushValues(lua_State* state, const Type& arg)
	{
		PushValue(state, arg);
	}

	template <typename First, typename ...Last>
	inline void PushValues(lua_State* state, const First& first, const Last& ...args)
	{
		PushValue(state, first);
		PushValues(state, args...);
	}

	template <typename Type>
	inline Type PopValue(lua_State* state)
	{
		Type result = TypeInfo<Type>::GetValue(state, -1);
		LuaPop(state, 1);
		return result;
	}
}
