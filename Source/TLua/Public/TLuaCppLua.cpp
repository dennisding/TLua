
#include "TLua.h"
#include "TLua.hpp"
#include "TLuaCppLua.hpp"

#include "CoreMinimal.h"

namespace TLua
{
	// static void AttrSetter(void* self, const FString&name, const ValueGetter &getter)
	static void AttrSetter(void* self, const FString& name, double fv)
	{
		if (self == nullptr) {
			return;
		}

		FString ClassName(TEXT("/Script/CppLua.CppLuaCharacter"));
		UClass* Class = LoadObject<UClass>(nullptr, *ClassName);
		if (Class) {
			FProperty* property = Class->FindPropertyByName(*name);
			if (property) {
				UE_LOG(Lua, Error, TEXT("set attr:%s"), *name);
				FFloatProperty* FloatProp = CastField<FFloatProperty>(property);
				void* ValuePtr = FloatProp->ContainerPtrToValuePtr<void>(self);
				FloatProp->SetFloatingPointPropertyValue(ValuePtr, (double)fv);
			}
		}
	}

	static void UpdateVTable(const FString& TypeId, const FString& Name)
	{
		//		FString ClassName(TEXT("/Script/CppLua.CppLuaCharacter"));
		UClass* Class = LoadObject<UClass>(nullptr, *TypeId, nullptr, LOAD_Quiet);
		if (Class == nullptr) {
			return;
		}

		FName AttrName(*Name);
		FProperty* Property = Class->FindPropertyByName(AttrName);
		if (Property) {
			// update property vtable
			// gp, getter, sp, setter
			//Call("_cpp_vtable_add_attr", name_, attr_name,
			//	GetProcessor(getter), getter,
			//	GetProcessor(setter), setter);
			Call("_lua_update_vtable", TypeId, Name,
				GetProcessor(AttrSetter), AttrSetter,
				GetProcessor(AttrSetter), AttrSetter);
			return;
		}

		UFunction* Function = Class->FindFunctionByName(AttrName);
		if (Function) {

		}
		else {
			// no attribute
		}
	}

	void RegisterCppLua()
	{
		RegisterCallback("update_vtable", UpdateVTable);
	}
}