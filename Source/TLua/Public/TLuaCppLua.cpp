
#include <map>
#include <string>

#include "TLua.h"
#include "TLua.hpp"
#include "TLuaCppLua.hpp"

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

namespace TLua
{
	struct AttributeInfo
	{
	public:
		std::function<void(UObject*, double)> Setter;
		std::function<double(UObject*)> Getter;
	};
	// 每个类只有一个vtable, 所以这里不考虑释放和内存泄漏问题
	class VTable
	{
	public:
		explicit VTable(const FString& TypeId) : Class(nullptr), Name(TypeId)
		{
			Class = LoadObject<UClass>(nullptr, *TypeId);
		}

		void AddAttribute(const FName& AttrName)
		{
			if (!Class) {
				EmptyAttribute(AttrName);
				return;
			}

			auto Finded = Attributes.Find(AttrName);
			if (Finded) {
				EmptyAttribute(AttrName);
				return;
			}

			FProperty *Property = Class->FindPropertyByName(AttrName);
			if (Property) {
				UpdateProperty(AttrName, Property);
				return;
			}

			UFunction* Function = Class->FindFunctionByName(AttrName);
			if (Function) {
				UE_LOG(Lua, Error, TEXT("update fun"));
				UpdateFunction(AttrName, Function);
				return;
			}
			EmptyAttribute(AttrName);
		}

	private:
		void EmptyAttribute(const FName& AttrName)
		{
			Attributes.Add(AttrName, nullptr);
			Call("_lua_update_vtable", Name, AttrName, 0);
		}

		void UpdateProperty(const FName& AttrName, FProperty* Property)
		{
			//UE_LOG(Lua, Error, TEXT("set attr:%s"), *name);
			//FFloatProperty* FloatProp = CastField<FFloatProperty>(property);
			//void* ValuePtr = FloatProp->ContainerPtrToValuePtr<void>(self);
			//FloatProp->SetFloatingPointPropertyValue(ValuePtr, (double)fv);

			FFloatProperty* FloatProp = CastField<FFloatProperty>(Property);
			
			AttributeInfo* Attribute = new AttributeInfo;
			Attribute->Getter = [FloatProp](UObject* obj) -> float {
				void* ValuePtr = FloatProp->ContainerPtrToValuePtr<void>(obj);
				return FloatProp->GetFloatingPointPropertyValue(ValuePtr);
				};

			Attribute->Setter = [FloatProp](UObject* obj, float value) {
					void* ValuePtr = FloatProp->ContainerPtrToValuePtr<void>(obj);
					FloatProp->SetFloatingPointPropertyValue(ValuePtr, value);
				};


			Call("_lua_update_vtable", Name, AttrName, Attribute,
				GetProcessor(GetValue), GetValue,
				GetProcessor(SetValue), SetValue);

			// Attributes[AttrName] = Attribute;
			Attributes.Add(AttrName, Attribute);
		}

		static void SetValue(void *self, void *attr, double value)
		{
			UObject* Obj = (UObject*)self;
			AttributeInfo* Info = (AttributeInfo*)attr;

			Info->Setter(Obj, (float)value);
		}

		static double GetValue(void* self, void* attr)
		{
			UObject* Obj = (UObject*)self;
			AttributeInfo* Info = (AttributeInfo*)attr;
			return Info->Getter(Obj);
		}

		void UpdateFunction(const FName& AttrName, UFunction* Function)
		{

		}
	private:
		TMap<FName, AttributeInfo*> Attributes;
		UClass* Class;
		FString Name;
	};

	class VTableMgr
	{
	public:
		static VTable* GetVTable(const FString& TypeId)
		{
			static VTableMgr Mgr;

			auto Finded = Mgr.VTables.Find(TypeId);
			if (Finded) {
				return *Finded;
			}
			// create new vtable
			VTable* NewVTable = new VTable(TypeId);
			// Mgr.VTables[TypeId] = NewVTable;
			Mgr.VTables.Add(TypeId, NewVTable);
			return NewVTable;
		}

//	private:
		TMap<FString, VTable*> VTables;
	};

	// static void AttrSetter(void* self, const FString&name, const ValueGetter &getter)
	static void AttrSetter(void* self, const FString& name, double fv)
	{
		if (self == nullptr) {
			return;
		}

	/*	UObject* Obj = (UObject*)self;
		VTable* Table = VTableMgr::GetVTable(Obj->GetName());

		Table->AddProperty(name);*/
		
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

	static void UpdateVTable(const FString& TypeId, const FName& Name)
	{
		//		FString ClassName(TEXT("/Script/CppLua.CppLuaCharacter"));
		UClass* Class = LoadObject<UClass>(nullptr, *TypeId, nullptr, LOAD_Quiet);
		if (Class == nullptr) {
			return;
		}

		VTable* Table = VTableMgr::GetVTable(TypeId);
		Table->AddAttribute(Name);

		//FName AttrName(*Name);
		//FProperty* Property = Class->FindPropertyByName(AttrName);
		//if (Property) {
		//	// update property vtable
		//	// gp, getter, sp, setter
		//	//Call("_cpp_vtable_add_attr", name_, attr_name,
		//	//	GetProcessor(getter), getter,
		//	//	GetProcessor(setter), setter);
		//	Call("_lua_update_vtable", TypeId, Name,
		//		GetProcessor(AttrSetter), AttrSetter,
		//		GetProcessor(AttrSetter), AttrSetter);
		//	return;
		//}

		//UFunction* Function = Class->FindFunctionByName(AttrName);
		//if (Function) {

		//}
		//else {
		//	// no attribute
		//}
	}

	void RegisterCppLua()
	{
		RegisterCallback("update_vtable", UpdateVTable);
	}
}