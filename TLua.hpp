#pragma once

#include <string>
#include <functional>
#include "Lua/lua.hpp"

#include "TLuaArgs.hpp"

namespace TLua
{
	void Init();
	void DoFile(const std::string &file_name);
	void DoString(const char* buff);
	lua_State* GetLuaState();
	bool CheckState(int r, lua_State* state);

	//inline void Call(const std::string& name)
	//{
	//	lua_State* state = GetLuaState();
	//	lua_getglobal(state, name.c_str());
	////	lua_pcall(state, 0, 0, 0);
	//	CheckState(lua_pcall(state, 0, 0, 0), state);
	//}

	template <typename ...Types>
	inline void Call(std::string&& name, Types&&... args)
	{
		lua_State* state = GetLuaState();
		lua_getglobal(state, name.c_str());
		PushValues(state, args...);

		CheckState(lua_pcall(state, sizeof...(Types), 0, 0), state);
	}

	template <typename R, typename ...Types>
	inline R Call(std::string &&name, Types&&... args)
	{ 
		lua_State* state = GetLuaState();
		lua_getglobal(state, name.c_str());
		PushValues(state, args...);

		CheckState(lua_pcall(state, sizeof...(Types), 1, 0), state);

		return PopValue<R>(state);
	}

	using LuaCFun = int (*)(lua_State *state);

	template <typename ...Types>
	struct TypeList{};

	template <int index, typename Type>
	struct Getter
	{
		inline static Type GetValue(lua_State* state)
		{
			// return (Type)lua_tonumber(state, index);
			return TLua::GetValue<Type>(state, index);
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

	template <typename ReturnType, typename FunType, typename ...Types>
	struct CallHelper
	{
		using ArgList = typename ExtendGetter<3, TypeList<>, Types...>::List;
		inline static ReturnType Call(FunType fun, lua_State *state)
		{
			if (lua_gettop(state) != sizeof...(Types) + 2) {
				lua_pushstring(state, "invalid function argument");
				lua_error(state);
				return ReturnType();
			}
			// CheckState(lua_gettop());
			return DoCall(fun, state, ArgList());
		}

		template <typename ...GetterType>
		inline static ReturnType DoCall(FunType fun, lua_State* state, TypeList<GetterType...> getter)
		{
			return fun(GetterType::GetValue(state)...);
		}
	};

	template <typename ...Types>
	int Processor(lua_State* state)
	{
		using FunType = void(*)(Types ...args);
		FunType fun = (FunType)lua_touserdata(state, 2);

		CallHelper<void, FunType, Types...>::Call(fun, state);

		return 0;
	}

	template <typename ReturnType, typename ...Types>
	int ProcessorWithReturn(lua_State* state)
	{
		using FunType = ReturnType(*)(Types ...args);
		FunType fun = (FunType)lua_touserdata(state, 2);

		// PushValue(state, fun());
		PushValue(state, CallHelper<ReturnType, FunType, Types...>::Call(fun, state));

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

	// bind the c++ object with lua
	struct VTableBase {};

	template <typename Type>
	struct VTable : VTableBase
	{
		template <typename ValueType>
		struct PointerHolder 
		{
			ValueType Type::* ptr_;
		};

		template <typename ValueType>
		struct AttrPtr
		{
			using Ptr = ValueType Type::*;
			AttrPtr(Ptr ptr) : ptr_(ptr) {}

			Ptr ptr_;
		};

		template <typename ReturnType, typename ...ArgTypes>
		struct MethodPtr
		{
			using Ptr = ReturnType(Type::*)(ArgTypes... args);
			MethodPtr(Ptr ptr) : ptr_(ptr) {}
			
			Ptr ptr_;
		};

		using VTableMap = std::map<std::string, VTableBase>;
		VTable(const std::string& name)
		{
			name_ = name;
			Call("_cpp_new_vtable", name); // create new vtable
		}

		template <typename ValueType>
		VTable &AddAttr(const char* attr_name, ValueType Type::* ptr)
		{
			auto setter = &SetAttr<ValueType>;
			auto getter = &GetAttr<ValueType>;
			Call("_cpp_vtable_add_attr", name_, attr_name, 
				GetProcessor(getter), getter,
				GetProcessor(setter), setter);

			attrs_[attr_name] = new AttrPtr<ValueType>(ptr);

			return *this;
		}

		template <typename ReturnType, typename ...ArgTypes>
		VTable& AddAttr(const char* method_name, ReturnType(Type::* ptr)(ArgTypes...args))
		{
			//
			auto method = &CallMethod<ReturnType, ArgTypes...>;

			Call("_cpp_vtable_add_method", name_, method_name, GetProcessor(method), method);

			methods_[method_name] = new MethodPtr<ReturnType, ArgTypes...>(ptr);

			return *this;
		}

		template <typename ValueType>
		static void SetAttr(std::string type_name, Type* self, std::string name, ValueType value)
		{
			using Ptr = AttrPtr<ValueType>*;
			auto ptr = (Ptr)attrs_[name];

			self->*(ptr->ptr_) = value;
		}

		template <typename ValueType>
		static ValueType GetAttr(std::string type_name, Type *self, std::string name)
		{
			using Ptr = AttrPtr<ValueType>*;
			auto ptr = (Ptr)attrs_[name];

			return self->*(ptr->ptr_);
		}

		template <typename ValueType, typename ...ArgTypes>
		static ValueType CallMethod(std::string type_name, Type* self, std::string name, ArgTypes...args)
		{
			using Ptr = MethodPtr<ValueType, ArgTypes...>*;
			auto ptr = (Ptr)methods_[name];

			return (self->*(ptr->ptr_))(args...);
		}

		//template <typename ...ArgTypes>
		//static void CallMethod(std::string type_name, Type* self, std::string name, ArgTypes...args)
		//{
		//	using Ptr = MethodPtr<void, ArgTypes...>*;
		//	auto ptr = (Ptr)methods_[name];

		//	self->*(ptr->ptr_)(args...);
		//}

		//static void CallMethod(std::string type_name, Type* self, std::string name)
		//{
		//}

		//template <typename ValueType>
		//static ValueType CallMethod(std::string type_name, Type* self, std::string name)
		//{
		//	using Ptr = MethodPtr<ValueType>*;
		//	auto ptr = (Ptr)methods_[name];

		//	return self->*(ptr->ptr_)();
		//}

		static std::map<std::string, void*> attrs_;
		static std::map<std::string, void*> methods_;
		static std::string name_;
	};

	template <typename Type>
	std::map<std::string, void*> VTable<Type>::attrs_;
	template <typename Type>
	std::map<std::string, void*> VTable<Type>::methods_;

	template <typename Type>
	std::string VTable<Type>::name_;

	template <typename Type>
	inline void Bind(Type* obj, const char* name)
	{
		Call("_cpp_bind_obj", obj, name);
	}

	template <typename Type>
	inline void Unbind(Type* obj)
	{
		Call("_cpp_unbind_obj", obj);
	}

	template <typename Type, typename ...ArgTypes>
	inline void CallMethod(Type obj, const char* name, ArgTypes... args)
	{
		Call("_cpp_obj_call", obj, name, args...);
	}

	template <typename ReturnType, typename Type, typename ...ArgTypes>
	inline ReturnType CallMethod(Type obj, const char* name, ArgTypes... args)
	{
		return Call<ReturnType>("_cpp_obj_call", obj, name, args...);
	}

	template <typename Type, typename ReturnType>
	void GetAttr(Type obj, const char* name, ReturnType& r)
	{
		r = Call<ReturnType>("_cpp_obj_getattr", obj, name);
	}

	template <typename Type, typename ValueType>
	void SetAttr(Type obj, const char* name, ValueType value)
	{
		Call("_cpp_obj_setattr", obj, name, value);
	}
}