#pragma once

#include <string>
#include "lua.hpp"

#include "TLuaArgs.hpp"

namespace TLua
{
	void Init();
	void DoFile(const std::string &file_name);
	lua_State* GetLuaState();

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
	struct TypeList{};

	template <int index, typename Type>
	struct Getter
	{
		inline static Type GetValue(lua_State* state)
		{
			return (Type)lua_tonumber(state, index);
		}
	};

	template <typename ListType, typename Value, int index>
	struct AppendGetter {};

	
	template <typename Value, int index, typename ...ListValue>
	struct AppendGetter<TypeList<ListValue...>, Value, index>
	{
		using List = TypeList<ListValue..., Getter<index, Value>>;
	};

	template <int index, typename ListType, typename ...Values>
	struct ExtendGetter
	{
		using List = ListType;
	};

	template <int index, typename ListType, typename Head, typename ...Tails>
	struct ExtendGetter<index, ListType, Head, Tails...>
	{
		using AppendedList = typename AppendGetter<ListType, Head, index>::List;
		using List = typename ExtendGetter<index + 1, AppendedList, Tails...>::List;
	};

	template <typename FunType, typename ...Types>
	struct CallHelper
	{
		using ArgList = typename ExtendGetter<3, TypeList<>, Types...>::List;
		inline static void Call(FunType fun, lua_State *state)
		{
			DoCall(fun, state, ArgList());
		}

		template <typename ...GetterType>
		inline static void DoCall(FunType fun, lua_State* state, TypeList<GetterType...> getter)
		{
			fun(GetterType::GetValue(state)...);
		}
	};

	template <typename ...Types>
	int Processor(lua_State* state)
	{
		using FunType = void(*)(Types ...args);
		FunType fun = (FunType)lua_touserdata(state, 2);

		CallHelper<FunType, Types...>::Call(fun, state);

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
	inline LuaCFun GetProcessor(R(*callback)(Args...args))
	{
		// have return value
		return &ProcessorWithReturn<R, Args...>;
	}

	template <typename ...Args>
	inline LuaCFun GetProcessor(void(*callback)(Args...args))
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