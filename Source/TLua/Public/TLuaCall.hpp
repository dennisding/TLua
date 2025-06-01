#pragma once

#include "Lua/lua.hpp"
#include "TLuaImp.hpp"
#include "TLuaTypes.hpp"

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
	inline R RCall(const char* name, const Types&... args)
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
		static Type FromLua(lua_State* state)
		{
			return TypeInfo<Type>::FromLua(state, index);
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
			return fun(GetterType::FromLua(state)...);
		}
	};

	template <typename ...Types>
	int FunProcessor(lua_State* state)
	{
		using FunType = void(*)(Types... args);
		FunType fun = TypeInfo<FunType>::FromLua(state, 2);

		CallHelper<void, FunType, Types...>::Call(fun, state);

		return 0;
	}

	template <typename ReturnType, typename ...Types>
	int FunProcessorWithReturn(lua_State* state)
	{
		using FunType = ReturnType(*)(Types... args);
		FunType fun = TypeInfo<FunType>::FromLua(state, 2);

		TypeInfo<ReturnType>::PushValue(state,
			CallHelper<ReturnType, FunType, Types...>::Call(fun, state));

		return 1;
	}

	template <typename R, typename ...Args>
	LuaCFun GetProcessor(R(*callback)(Args...args))
	{
		return &FunProcessorWithReturn<R, Args...>;
	}

	template <typename ...Args>
	LuaCFun GetProcessor(void(*callback)(Args...args))
	{
		// no return value
		return &FunProcessor<Args...>;
	}

	template <typename R, typename ...Args>
	void RegisterCallback(const char* name, R(*callback)(Args... args))
	{
		RegisterCallbackImp(name, GetProcessor(callback), callback);
	}

	template <typename Type, typename ReturnType, typename ...ArgTypes>
	class MethodContext
	{
		using MethodType = ReturnType(Type::*)(ArgTypes...);
		using ArgList = typename ExtendGetter<3, TypeList<>, ArgTypes...>::List;
	public:
		MethodContext(MethodType InMethod) : Method(InMethod)
		{
		}

		// _callback(self, context, args...)
		static int Callback(lua_State* State)
		{
			using ContextType = MethodContext<Type, ReturnType, ArgTypes...>;
			UObject* Object = (UObject*)LuaGetUserData(State, 1);
			Type* Self = Cast<Type>(Object);
			if (!Self) {
				return 0;
			}

			ContextType* Context = (ContextType*)LuaGetUserData(State, 2);

			Context->DoCall(Self, State, ArgList());

			return 0;
		}

		// _callback(self, context, args...)
		static int CallbackWithReturn(lua_State* State)
		{
			using ContextType = MethodContext<Type, ReturnType, ArgTypes...>;
			UObject* Object = (UObject*)LuaGetUserData(State, 1);
			Type* Self = Cast<Type>(Object);
			if (!Self) {
				return 0;
			}

			ContextType* Context = (ContextType*)LuaGetUserData(State, 2);

			TypeInfo<ReturnType>::ToLua(State, Context->DoCall(Self, State, ArgList()));

			return 1;
		}

		template <typename ...GetterType>
		ReturnType DoCall(Type* Self, lua_State* State, TypeList<GetterType...> Getter)
		{
			return (Self->*Method)(GetterType::FromLua(State)...);
		}

		inline static LuaCFun GetCallback()
		{
			return GetCallbackImp(std::is_same<ReturnType, void>());
		}

	private:
		inline static LuaCFun GetCallbackImp(std::true_type is_void)
		{
			return Callback;
		}

		inline static LuaCFun GetCallbackImp(std::false_type not_void)
		{
			return CallbackWithReturn;
		}

	public:
		MethodType Method;
	};

	template <typename Type, typename ReturnType, typename ...ArgTypes>
	void ActorMethod(const char* Class, const char* Name, ReturnType (Type::*Method)(ArgTypes...Args))
	{
		auto Context = new MethodContext<Type, ReturnType, ArgTypes...>(Method);

		Call("_lua_actor_method", Class, Name, Context->GetCallback(), (void*)Context);
	}

	template <typename Type, typename ReturnType, typename ...ArgTypes>
	void ComponentMethod(const char* Component, const char* Name,
		ReturnType(Type::* Method)(ArgTypes...Args))
	{
		auto Context = new MethodContext<Type, ReturnType, ArgTypes...>(Method);

		Call("_lua_component_method", Component, Name, Context->GetCallback(), (void*)Context);
	}

	void RegisterUnreal();
}