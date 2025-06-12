
#include "TLuaRootObject.h"

#include "TLua.h"
#include "TLua.hpp"

UTLuaCallback::UTLuaCallback()
{
}

UTLuaCallback::~UTLuaCallback()
{
	check(IsInGameThread());

	lua_State* State = TLua::GetLuaState();
	lua_pushlightuserdata(State, this);
	lua_pushnil(State);
	lua_settable(State, LUA_REGISTRYINDEX);
}

void UTLuaCallback::Bind(int AbsIndex)
{
	lua_State* State = TLua::GetLuaState();
	lua_pushlightuserdata(State, this);
	lua_pushvalue(State, AbsIndex);
	lua_settable(State, LUA_REGISTRYINDEX);
}

void UTLuaCallback::Callback()
{
	lua_State* State = TLua::GetLuaState();
	lua_getglobal(State, TLUA_TRACE_CALL_NAME);
	lua_pushlightuserdata(State, this);
	lua_gettable(State, LUA_REGISTRYINDEX);

	lua_pcall(State, 1, 0, 0);
}

UTLuaRootObject::UTLuaRootObject()
{
	Delegate.BindUFunction(this, TEXT("SayHello"));
	OneDelegate.BindUFunction(this, TEXT("OneReturn"));
//	Delegate.ExecuteIfBound();
//	OneMultiDelegate.Add(&UTLuaRootObject::OneReturn);
	// OneMultiDelegate.AddRaw(this, &UTLuaRootObject::OneReturn);
//	OneMultiDelegate.AddUObject(this, &UTLuaRootObject::OneReturn);
	OneMultiDelegate.AddDynamic(this, &UTLuaRootObject::OneReturn);

	OneMultiDelegate.Broadcast(1024);
}

void UTLuaRootObject::SayHello()
{
	UE_LOG(Lua, Warning, TEXT("hello root object"));
}

void UTLuaRootObject::OneReturn(int IntValue)
{
	UE_LOG(Lua, Warning, TEXT("OneReturn: %d"), IntValue);
}
