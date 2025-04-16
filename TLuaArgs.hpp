#pragma once

namespace TLua
{
	// push cpp value to lua state
	inline void PushValue(lua_State* state, int iv)
	{
		lua_pushinteger(state, iv);
	}

	inline void PushValue(lua_State* state, const char* buff)
	{
		lua_pushstring(state, buff);
	}

	inline void PushValue(lua_State* state, const std::string& name)
	{
		lua_pushstring(state, name.c_str());
	}

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

	// get values from lua state
	template <typename ReturnType>
	inline ReturnType GetValue(lua_State *state, int index = -1) {}

	template <>
	inline bool GetValue(lua_State* state, int index)
	{
		return lua_toboolean(state, index) != 1;
	}

	template <>
	inline int GetValue(lua_State* state, int index)
	{
		return (int)lua_tointeger(state, index);
	}

	template <>
	inline float GetValue(lua_State* state, int index)
	{
		return (float)lua_tonumber(state, index);
	}

	template <>
	inline double GetValue(lua_State* state, int index)
	{
		return (double)lua_tonumber(state, index);
	}

	template <>
	inline std::string GetValue(lua_State* state, int index)
	{
		return std::move(std::string(lua_tostring(state, index)));
	}

	// pop value from lua state
	template <typename ReturnType>
	inline ReturnType PopValue(lua_State* state) 
	{
		ReturnType result = GetValue<ReturnType>(state, -1);
		lua_pop(state, 1);

		return std::move(result);
	}
}