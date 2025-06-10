
#include "TLuaRootObject.h"

#include "TLua.h"
#include "TLua.hpp"

UTLuaRootObject::UTLuaRootObject()
{
}

void UTLuaRootObject::SayHello()
{
	UE_LOG(Lua, Warning, TEXT("hello root object"));
}
