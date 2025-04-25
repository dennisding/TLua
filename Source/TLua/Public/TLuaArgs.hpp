#pragma once

#include <map>
#include <vector>

#include "CoreMinimal.h"

namespace TLua
{
	TLua_API void* LuaGetUserData(lua_State* state, int index);
	TLua_API int LuaGetInteger(lua_State* state, int index);
	TLua_API double LuaGetNumber(lua_State* state, int index);
	TLua_API std::string LuaGetString(lua_State* state, int index);
	TLua_API const char* LuaGetLString(lua_State* state, int index, size_t& size);

	// push value
	TLua_API void LuaPushInteger(lua_State* state, int iv);
	TLua_API void LuaPushNumber(lua_State* state, double number);
	TLua_API void LuaPushString(lua_State* state, const char* str);
	TLua_API void LuaPushLString(lua_State* state, const char *buff, int size);
	TLua_API void LuaPushUserData(lua_State* state, void *user_data);
	TLua_API void LuaPushNil(lua_State* state);

	TLua_API void LuaNewTable(lua_State* state);
	TLua_API void LuaSetTable(lua_State* state, int index);
	TLua_API void LuaGetTable(lua_State* state, int index);
	TLua_API void LuaLen(lua_State* state, int index);
	TLua_API int LuaNext(lua_State* state, int index);
	
	TLua_API void LuaPop(lua_State* state, int num);
	TLua_API int LuaAbsIndex(lua_State* state, int index);

	// push cpp value to lua state
	inline void PushValue(lua_State* state, int iv)
	{
		LuaPushInteger(state, iv);
	}

	inline void PushValue(lua_State* state, double value)
	{
		LuaPushNumber(state, value);
	}

	inline void PushValue(lua_State* state, const char* buff)
	{
		LuaPushString(state, buff);
	}

	inline void PushValue(lua_State* state, const std::string& name)
	{
		LuaPushString(state, name.c_str());
	}

	inline void PushValue(lua_State* state, const void* ptr)
	{
		LuaPushUserData(state, (void *)ptr);
	}

	inline void PushValue(lua_State* state, const FString& str)
	{
		LuaPushLString(state, (const char*)(*str), str.NumBytesWithoutNull());
	}

	inline void PushValue(lua_State* state, const TArray<FString>& values)
	{
		LuaNewTable(state);
		for (int i = 0; i < values.Num(); ++i) {
			PushValue(state, i + 1);
			PushValue(state, values[i]);
			LuaSetTable(state, -3);
		}
	}

	template <typename ValueType>
	inline void PushValue(lua_State* state, const std::vector<ValueType>& values)
	{
		LuaNewTable(state);
		for (int i = 0; i < values.size(); ++i) {
			PushValue(state, i + 1);
			PushValue(state, values[i]); // push value
			LuaSetTable(state, -3);
		}
	}

	template <typename KeyType, typename ValueType>
	inline void PushValue(lua_State* state, const std::map<KeyType, ValueType>& values)
	{
		LuaNewTable(state);
		for (auto &iter : values) {
			PushValue(state, iter.first);
			PushValue(state, iter.second);
			LuaSetTable(state, -3);
		}
	}

	inline void PushValues(lua_State* state) {}

	template <typename Type>
	inline void PushValues(lua_State* state, const Type &arg)
	{
		PushValue(state, arg);
	}

	template <typename First, typename ...Last>
	inline void PushValues(lua_State* state, const First &first, const Last& ...args)
	{
		PushValue(state, first);
		PushValues(state, args...);
	}

	//// get values from lua state
	// forward declare
	template <typename ReturnType>
	inline ReturnType PopValue(lua_State* state);

	template <typename ReturnType>
	inline ReturnType GetValue(lua_State* state, int index);

	template <typename ReturnType>
	struct ValueGetter {};

	template <>
	struct ValueGetter<int>
	{
		inline static int GetValue(lua_State* state, int index)
		{
			return (int)LuaGetInteger(state, index);
		}
	};

	template <>
	struct ValueGetter<double>
	{
		inline static double GetValue(lua_State* state, int index)
		{
			return LuaGetNumber(state, index);
		}
	};

	template <>
	struct ValueGetter<std::string>
	{
		inline static std::string GetValue(lua_State* state, int index)
		{
			return LuaGetString(state, index);
		}
	};

	template <>
	struct ValueGetter<FString>
	{
		inline static FString GetValue(lua_State* state, int index)
		{
			size_t size = 0;
			const TCHAR* buff = (const TCHAR*)LuaGetLString(state, index, size);

			return FString(size/sizeof(TCHAR), buff);
		}
	};

	template <typename Type>
	struct ValueGetter<Type*>
	{
		inline static Type* GetValue(lua_State* state, int index)
		{
			return (Type *)LuaGetUserData(state, index);
		}
	};

	template <typename ValueType>
	struct ValueGetter<TArray<ValueType>>
	{
		using ReturnType = TArray<ValueType>;

		inline static ReturnType GetValue(lua_State* state, int index)
		{
			// check this is a table
			int abs_index = LuaAbsIndex(state, index);

			// get table length
			LuaLen(state, abs_index);
			int table_size = PopValue<int>(state);

			ReturnType result;
			result.Reserve(table_size);
			for (size_t i = 1; i <= table_size; ++i) {
				PushValue(state, i);
				LuaGetTable(state, abs_index);
				result.Emplace(PopValue<ValueType>(state));
			}
			return result;
		}
	};

	template <typename ValueType>
	struct ValueGetter<std::vector<ValueType>>
	{
		using ReturnType = std::vector<ValueType>;

		inline static ReturnType GetValue(lua_State* state, int index)
		{
			// check this is a table
			int abs_index = LuaAbsIndex(state, index);
			
			// get table length
			LuaLen(state, abs_index);
			int table_size = PopValue<int>(state);

			ReturnType result;
			result.reserve(table_size);
			for (size_t i = 1; i <= table_size; ++i) {
				PushValue(state, i);
				LuaGetTable(state, abs_index);
				result.emplace_back(PopValue<ValueType>(state));
			}
			return result;
		}
	};

	template <typename KeyType, typename ValueType>
	struct ValueGetter<std::map<KeyType, ValueType>>
	{
		using ReturnType = std::map<KeyType, ValueType>;

		inline static ReturnType GetValue(lua_State* state, int index)
		{
			ReturnType result;

			int abs_index = LuaAbsIndex(state, index);
			LuaPushNil(state);
			while (LuaNext(state, abs_index) != 0) {
				KeyType key = TLua::GetValue<KeyType>(state, -2);
				result.emplace(std::make_pair(key, PopValue<ValueType>(state)));
			}

			return result;
		}
	};

	template <typename ReturnType>
	inline ReturnType GetValue(lua_State* state, int index)
	{
		return ValueGetter<ReturnType>::GetValue(state, index);
	}

	// pop value from lua state
	template <typename ReturnType>
	inline ReturnType PopValue(lua_State* state) 
	{
		ReturnType result = ValueGetter<ReturnType>::GetValue(state, -1);
		LuaPop(state, 1);

		return result;
	}
}
