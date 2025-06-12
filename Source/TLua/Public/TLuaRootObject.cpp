
#include "TLuaRootObject.h"

#include "TLua.h"
#include "TLua.hpp"

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
