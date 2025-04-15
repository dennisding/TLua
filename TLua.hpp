#pragma once

#include <string>
#include "lua.hpp"

#include "TLuaArgs.hpp"

namespace TLua
{
	void Init();
	void DoFile(const std::string &file_name);
	lua_State* GetLuaState();

	inline void PushValue(lua_State* state, int iv)
	{
		lua_pushinteger(state, iv);
	}

	inline void PushValue(lua_State* state, const std::string& name)
	{
		lua_pushstring(state, name.c_str());
	}

	template <typename ReturnType>
	inline ReturnType PopValue(lua_State* state)
	{

	}

	template <>
	inline int PopValue(lua_State* state)
	{
		double result = lua_tonumber(state, -1);
		lua_pop(state, 1);
		return int(result);
	}

	template <>
	inline std::string PopValue(lua_State* state)
	{
		size_t size = 0;
		const char* buff = lua_tolstring(state, -1, &size);
		std::string result(buff);

		lua_pop(state, 1);
		return std::move(result);
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

	inline void Call(const std::string& name)
	{
		lua_State* state = GetLuaState();
		lua_getglobal(state, name.c_str());
		lua_call(state, 0, 0);
	}

	template <typename ...Types>
	inline void Call(const std::string& name, Types... args)
	{
		lua_State* state = GetLuaState();
		lua_getglobal(state, name.c_str());
		PushValues(state, args...);

		lua_call(state, sizeof...(Types), 0);
	}

	template <typename R, typename ...Types>
	inline R Call(const std::string &name, Types... args)
	{ 
		lua_State* state = GetLuaState();
		lua_getglobal(state, name.c_str());
		PushValues(state, args...);

		lua_call(state, sizeof...(Types), 1);

		return std::move(PopValue<R>(state));
	}

	using LuaCFun = int (*)(lua_State *state);

	template <typename ...Types>
	int Processor(lua_State* state)
	{
		using FunType = void(*)(Types ...args);
		FunType fun = (FunType)lua_touserdata(state, 2);

		// unpack the argument ...
		fun();
		// CallHelper<FunType, Types ...>::Call(state, fun))
		// fun(GetValue(state, 3), GetValue(state, 4), GetValue(state, 5)...)

		return 0;
	}

	template <typename R, typename ...Types>
	int ProcessorWithReturn(lua_State* state)
	{
		using FunType = R(*)(Types ...args);
		FunType fun = (FunType)lua_touserdata(state, 2);

		PushValue(state, fun());

		return 1;
	}

	template <typename R, typename ...Args>
	LuaCFun GetProcessor(R(*callback)(Args...args))
	{
		// have return value
		return &ProcessorWithReturn<R, Args...>;
	}

	template <typename ...Args>
	LuaCFun GetProcessor(void(*callback)(Args...args))
	{
		// no return value
		return &Processor<Args...>;
	}

	template <typename R, typename ...Args>
	inline void RegisterCallback(const char* name, R(*callback)(Args... args))
	{
		lua_State* state = GetLuaState();

		lua_getglobal(state, "_register_callback");
		lua_pushstring(state, name);
		lua_pushlightuserdata(state, GetProcessor(callback));
		lua_pushlightuserdata(state, callback);

		lua_call(state, 3, 0);
	}

}