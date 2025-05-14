#pragma once

#include "Lua/lua.hpp"
#include "TLuaImp.hpp"
#include "TLuaTypes.hpp"

#define TLUA_TRACE_CALL_NAME "trace_call"

namespace TLua
{
	inline void Call(const char* name)
	{
		lua_State* state = GetLuaState();

		LuaGetGlobal(state, TLUA_TRACE_CALL_NAME);
		LuaGetGlobal(state, name);
		LuaCall(state, 1);
	}

	template <typename ...Types>
	inline void Call(const char* name, const Types&... args)
	{
		lua_State* state = GetLuaState();

		LuaGetGlobal(state, TLUA_TRACE_CALL_NAME);
		LuaGetGlobal(state, name);
		PushValues(state, args...);
		LuaCall(state, sizeof...(Types) + 1);
	}

	template <typename R, typename ...Types>
	inline R Call(const char* name, const Types&... args)
	{
		lua_State* state = GetLuaState();

		LuaGetGlobal(state, TLUA_TRACE_CALL_NAME);
		LuaGetGlobal(state, name);
		PushValues(state, args...);
		LuaCall(state, sizeof...(Types) + 1, 1);

		return PopValue<R>(state);
	}

	template <typename ...Types>
	inline void CallMethod(UObject* Object, const char* Name, const Types&... Args)
	{
		Call("_lua_call_method", (void*)Object, Name, Args...);
	}

	using LuaCFun = int (*)(lua_State* state);

	template <typename ...Types>
	struct TypeList {};

	template <int index, typename Type>
	struct Getter
	{
		static Type GetValue(lua_State* state)
		{
			return TypeInfo<Type>::GetValue(state, index);
		}
	};

	template <typename ListType, typename Value, int index>
	struct AppendGetter {};


	template <typename ValueType, int index, typename ...ListValue>
	struct AppendGetter<TypeList<ListValue...>, ValueType, index>
	{
		using RawValueType = std::remove_cv_t<typename std::remove_reference<ValueType>::type>;
		using List = TypeList<ListValue..., Getter<index, RawValueType>>;
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

	template <typename ReturnType, typename FunType, typename ...Types>
	struct CallHelper
	{
		using ArgList = typename ExtendGetter<3, TypeList<>, Types...>::List;
		static ReturnType Call(FunType fun, lua_State* state)
		{
			if (LuaGetTop(state) != sizeof...(Types) + 2) {
				LuaError(state, "invalid function argument");
				return ReturnType();
			}

			return DoCall(fun, state, ArgList());
		}

		template <typename ...GetterType>
		static ReturnType DoCall(FunType fun, lua_State* state, TypeList<GetterType...> getter)
		{
			return fun(GetterType::GetValue(state)...);
		}
	};

	template <typename ...Types>
	int Processor(lua_State* state)
	{
		using FunType = void(*)(Types... args);
		FunType fun = TypeInfo<FunType>::GetValue(state, 2);

		CallHelper<void, FunType, Types...>::Call(fun, state);

		return 0;
	}

	template <typename ReturnType, typename ...Types>
	int ProcessorWithReturn(lua_State* state)
	{
		using FunType = ReturnType(*)(Types... args);
		FunType fun = TypeInfo<FunType>::GetValue(state, 2);

		TypeInfo<ReturnType>::PushValue(state,
			CallHelper<ReturnType, FunType, Types...>::Call(fun, state));

		return 1;
	}

	template <typename R, typename ...Args>
	LuaCFun GetProcessor(R(*callback)(Args...args))
	{
		return &ProcessorWithReturn<R, Args...>;
	}

	template <typename ...Args>
	LuaCFun GetProcessor(void(*callback)(Args...args))
	{
		// no return value
		return &Processor<Args...>;
	}

	template <typename R, typename ...Args>
	void RegisterCallback(const char* name, R(*callback)(Args... args))
	{
		RegisterCallbackImp(name, GetProcessor(callback), callback);
	}
}