#pragma once

#include <string>
#include <type_traits>
#include <functional>

#include "Lua/lua.hpp"
#include "TLuaArgs.hpp"

namespace TLua
{
	TLua_API void Init();
	TLua_API void DoFile(const char *name);
	TLua_API void DoString(const char* buff);
	TLua_API lua_State* GetLuaState();
	TLua_API bool CheckState(int r, lua_State* state);
	TLua_API void RegisterCallback(const char *name, void* processor, void* callback);
	TLua_API void LuaGetGlobal(lua_State* state, const char *name);
	TLua_API void LuaCall(lua_State* state, int arg_num);

	template <typename ...Types>
	void Call(const char *name, const Types&... args)
	{
		lua_State* state = GetLuaState();
		LuaGetGlobal(state, name);
		PushValues(state, args...);
		LuaCall(state, sizeof...(Types));
		//lua_State* state = GetLuaState();
		//lua_getglobal(state, name.c_str());
		//PushValues(state, args...);

		//CheckState(lua_pcall(state, sizeof...(Types), 0, 0), state);
	}

	template <typename R, typename ...Types>
	R Call(std::string &&name, const Types&... args)
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
		static Type GetValue(lua_State* state)
		{
			return TLua::GetValue<Type>(state, index);
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
		static ReturnType Call(FunType fun, lua_State *state)
		{
			if (lua_gettop(state) != sizeof...(Types) + 2) {
				lua_pushstring(state, "invalid function argument");
				lua_error(state);
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
		FunType fun = (FunType)lua_touserdata(state, 2);

		CallHelper<void, FunType, Types...>::Call(fun, state);

		return 0;
	}

	template <typename ReturnType, typename ...Types>
	int ProcessorWithReturn(lua_State* state)
	{
		using FunType = ReturnType(*)(Types... args);
		FunType fun = (FunType)lua_touserdata(state, 2);

		PushValue(state, CallHelper<ReturnType, FunType, Types...>::Call(fun, state));

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
		RegisterCallback(name, GetProcessor(callback), callback);
	}

	// bind the c++ object with lua
	template <typename Type>
	struct VTableImp
	{
        template <typename PtrType, typename ValueTypeIn = void, typename ...ArgTypes>
        struct AttrHolderImp
        {
            using Ptr = PtrType;
            using ValueType = ValueTypeIn;

            AttrHolderImp(Ptr ptr) : ptr_(ptr) {}

            Ptr ptr_;
        };

		VTableImp(const std::string& name)
		{
			name_ = name;
			Call("_cpp_new_vtable", name); // create new vtable
		}

		template <typename ValueType>
		VTableImp &AddAttr(const char* attr_name, ValueType Type::* ptr)
		{
            using AttrHolder = AttrHolderImp<ValueType Type::*, ValueType>;
            auto getter = &GetAttr<AttrHolder>;
            auto setter = &SetAttr<AttrHolder>;

            attrs_[attr_name] = new AttrHolder(ptr);

            Call("_cpp_vtable_add_attr", name_, attr_name,
                GetProcessor(getter), getter,
                GetProcessor(setter), setter);

			return *this;
		}

        template <typename ReturnType, typename ...ArgTypes>
        VTableImp& AddAttr(const char* method_name, ReturnType (Type::*ptr)(ArgTypes... args) const)
        {
            using PtrType = ReturnType(Type::*)(ArgTypes... args);
            return AddAttr(method_name, (PtrType)ptr);
        }

        template <typename ReturnType, typename ...ArgTypes>
        VTableImp& AddAttr(const char* method_name, ReturnType(Type::* ptr)(ArgTypes... args) )
        {
            using PtrType = ReturnType(Type::*)(ArgTypes... args);
            using Holder = AttrHolderImp<PtrType, ReturnType>;

            auto method = &CallMethod<Holder, ArgTypes...>;
            attrs_[method_name] = new Holder(ptr);
            Call("_cpp_vtable_add_method", name_, method_name, GetProcessor(method), method);

            return *this;
        }

        template <typename AttrHolder>
        static void SetAttr(const std::string& type_name, Type* self, const std::string& name,
            const typename AttrHolder::ValueType& value)
        {
            AttrHolder* holder = (AttrHolder*)attrs_[name];
            self->*(holder->ptr_) = value;
        }

        template <typename AttrHolder>
        static typename AttrHolder::ValueType
            GetAttr(const std::string& type_name, Type* self, const std::string& name)
        {
            AttrHolder* ptr = (AttrHolder*)attrs_[name];

            return self->*(ptr->ptr_);
        }

        template <typename HolderType, typename ...ArgTypes>
        static typename HolderType::ValueType CallMethod(
            const std::string& type_name,
            Type* self,
            const std::string& name,
            ArgTypes... args
        )
        {
            HolderType* holder = (HolderType*)attrs_[name];
            return (self->*(holder->ptr_))(args...);
        }

		static std::map<std::string, void*> attrs_;
		static std::string name_;
	};

    template <typename Type>
    VTableImp<Type> &VTable(const char* name)
    {
        static VTableImp<Type> vtable(name);
        return vtable;
    }

	template <typename Type>
	std::map<std::string, void*> VTableImp<Type>::attrs_;

	template <typename Type>
	std::string VTableImp<Type>::name_;

	template <typename Type>
	void Bind(Type* obj, const char* name)
	{
		Call("_cpp_bind_obj", obj, name);
	}

	template <typename Type>
	void Unbind(Type* obj)
	{
		Call("_cpp_unbind_obj", obj);
	}

	template <typename Type, typename ...ArgTypes>
	void CallMethod(Type obj, const char* name, ArgTypes... args)
	{
		Call("_cpp_obj_call", obj, name, args...);
	}

	template <typename ReturnType, typename Type, typename ...ArgTypes>
	ReturnType CallMethod(Type obj, const char* name, ArgTypes... args)
	{
		return Call<ReturnType>("_cpp_obj_call", obj, name, args...);
	}

	template <typename Type, typename ReturnType>
	void GetAttr(Type obj, const char* name, ReturnType& r)
	{
		r = Call<ReturnType>("_cpp_obj_getattr", obj, name);
	}

    template <typename ReturnType, typename Type>
    ReturnType GetAttr(Type obj, const char *name)
    {
        return Call<ReturnType>("_cpp_obj_getattr", obj, name);
    }

	template <typename Type, typename ValueType>
	void SetAttr(Type obj, const char* name, ValueType value)
	{
		Call("_cpp_obj_setattr", obj, name, value);
	}
}
