#pragma once

#include "CoreMinimal.h"

#include "TLuaCall.hpp"

namespace TLua
{
	TLUA_API class LuaObject
	{
	public:
		inline LuaObject() : Owner(nullptr) {}
		LuaObject(const LuaObject&) = delete;
		LuaObject(const LuaObject&&) = delete;
		const LuaObject& operator=(const LuaObject&&) = delete;

		inline LuaObject(UObject* InOwner, bool IsComponent = false) : Owner(InOwner)
		{
			TLua::Call("_lua_bind_obj", (void*)Owner, (void*)Owner->GetClass(), IsComponent);
		}

		inline ~LuaObject()
		{
			TLua::Call("_lua_unbind_obj", (void*)Owner);
		}

		template <typename ...ArgTypes>
		void TryCall(const char* Name, const ArgTypes&... Args)
		{
			TLua::Call("_lua_tcall", (void*)Owner, Name, Args...);
		}

		template <typename ...ArgTypes>
		void Call(const char* Name, const ArgTypes&... Args)
		{
			TLua::Call("_lua_call", (void*)Owner, Name, Args...);
		}

		template <typename ReturnType, typename ...ArgTypes>
		ReturnType RCall(const char* Name, const ArgTypes&... Args)
		{
			return TLua::RCall<ReturnType>("_lua_call", (void*)Owner, Name, Args...);
		}

	private:
		UObject* Owner;
	};
}