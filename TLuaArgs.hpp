#pragma once

#include <map>
#include <vector>

namespace TLua
{
	// push cpp value to lua state
	inline void PushValue(lua_State* state, int iv)
	{
		lua_pushinteger(state, iv);
	}

	inline void PushValue(lua_State* state, double value)
	{
		lua_pushnumber(state, value);
	}

	inline void PushValue(lua_State* state, const char* buff)
	{
		lua_pushstring(state, buff);
	}

	inline void PushValue(lua_State* state, const std::string& name)
	{
		lua_pushstring(state, name.c_str());
	}

	template <typename ValueType>
	inline void PushValue(lua_State* state, std::vector<ValueType>& values)
	{
		lua_newtable(state);
		for (int i = 0; i < values.size(); ++i) {
			lua_pushnumber(state, i + 1); // push key
			PushValue(state, values[i]); // push value
			lua_settable(state, -3);
		}
	}

	template <typename KeyType, typename ValueType>
	inline void PushValue(lua_State* state, std::map<KeyType, ValueType>& values)
	{
		lua_newtable(state);
		for (auto &iter : values) {
			PushValue(state, iter.first);
			PushValue(state, iter.second);
			lua_settable(state, -3);
		}
	}

	inline void PushValues(lua_State* state) {}

	template <typename Type>
	inline void PushValues(lua_State* state, Type arg)
	{
		PushValue(state, arg);
	}

	template <typename First, typename ...Last>
	inline void PushValues(lua_State* state, First first, Last ...args)
	{
		PushValue(state, first);
		PushValues(state, args...);
	}

	//// get values from lua state
	//template <typename ReturnType>
	//inline ReturnType GetValue(lua_State *state, int index = -1) {}

	//template <>
	//inline bool GetValue(lua_State* state, int index)
	//{
	//	return lua_toboolean(state, index) != 1;
	//}

	//template <>
	//inline int GetValue(lua_State* state, int index)
	//{
	//	return (int)lua_tointeger(state, index);
	//}

	//template <>
	//inline float GetValue(lua_State* state, int index)
	//{
	//	return (float)lua_tonumber(state, index);
	//}

	//template <>
	//inline double GetValue(lua_State* state, int index)
	//{
	//	return (double)lua_tonumber(state, index);
	//}

	//template <>
	//inline std::string GetValue(lua_State* state, int index)
	//{
	//	return std::move(std::string(lua_tostring(state, index)));
	//}

	// template spacialize
	template <typename ReturnType>
	struct ValueGetter {};

	template <>
	struct ValueGetter<int>
	{
		inline static int GetValue(lua_State* state, int index)
		{
			return (int)lua_tointeger(state, index);
		}
	};

	template <>
	struct ValueGetter<double>
	{
		inline static double GetValue(lua_State* state, int index)
		{
			return (double)lua_tointeger(state, index);
		}
	};

	template <>
	struct ValueGetter<std::string>
	{
		inline static std::string GetValue(lua_State* state, int index)
		{
			return std::move(std::string(lua_tostring(state, index)));
		}
	};

	// forward declare
	template <typename ReturnType>
	inline ReturnType PopValue(lua_State* state);

	template <typename ReturnType>
	inline ReturnType GetValue(lua_State* state, int index);

	template <typename ValueType>
	struct ValueGetter<std::vector<ValueType>>
	{
		using ReturnType = std::vector<ValueType>;

		inline static ReturnType GetValue(lua_State* state, int index)
		{
			// check this is a table
			int abs_index = lua_absindex(state, index);
			
			// get table length
			lua_len(state, abs_index);
			int table_size = (int)lua_tointeger(state, -1);
			lua_pop(state, 1);

			ReturnType result;
			for (size_t i = 1; i <= table_size; ++i) {
				lua_pushinteger(state, i);
				lua_gettable(state, abs_index);
				result.push_back(PopValue<ValueType>(state));
			}
			return std::move(result);
		}
	};

	template <typename KeyType, typename ValueType>
	struct ValueGetter<std::map<KeyType, ValueType>>
	{
		using ReturnType = std::map<KeyType, ValueType>;

		inline static ReturnType GetValue(lua_State* state, int index)
		{
			ReturnType result;

			int abs_index = lua_absindex(state, index);
			lua_pushnil(state); // first key
			while (lua_next(state, abs_index) != 0) {
				ValueType value = PopValue<ValueType>(state);
				KeyType key = TLua::GetValue<KeyType>(state, -1);
				result[key] = value;
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
		lua_pop(state, 1);

		return result;
	}
}