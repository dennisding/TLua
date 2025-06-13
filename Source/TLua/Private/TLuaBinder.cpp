#include "TLuaBinder.h"


#include "TLua.hpp"

UTLuaBinder::UTLuaBinder() 
	: Owner(nullptr)
{
//	Bind(InOwner, IsComponent);
}

UTLuaBinder::~UTLuaBinder()
{
	Unbind();
}

void UTLuaBinder::BindActor(UObject* InOwner)
{
	Bind(InOwner, false);
}

void UTLuaBinder::BindComponent(UObject* InOwner)
{
	Bind(InOwner, true);
}

void UTLuaBinder::Bind(UObject* InOwner, bool IsComponent)
{
	Owner = InOwner;
	TLua::Call("_lua_bind_obj", (void*)Owner, (void*)Owner->GetClass(), IsComponent);
}

void UTLuaBinder::Unbind()
{
	TLua::Call("_lua_unbind_obj", (void*)Owner);
	Owner = nullptr;
}