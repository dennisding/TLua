#include <map>
#include <string>

#include "TLua.h"
#include "TLua.hpp"
#include "TLuaCppLua.hpp"
#include "TLuaTypes.hpp"
#include "TLuaProperty.hpp"

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

namespace TLua
{
	struct AttributeInfo
	{
	public:
		std::function<void(UObject*, lua_State*)> Setter;
		std::function<void(UObject*, lua_State*)> Getter;
	};

	// because every class have only one vtable
	// we don't consider the memory leak here.
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
			if (Finded) { // Error, you can only set the attribute once
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
//				UpdateVTable(AttrName, Function);
				return;
			}
			EmptyAttribute(AttrName);
		}

	private:
		void EmptyAttribute(const FName& AttrName)
		{
			Attributes.Add(AttrName, nullptr);
			Call("_lua_update_vtable_attr", Name, AttrName, 0);
		}

		template <typename Type>
		void UpdateVTable(const FName& AttrName, Type* Property)
		{
			AttributeInfo* Attribute = GenAttributeInfo(Property);

			Call("_lua_update_vtable_attr", Name, AttrName, Attribute);
		}

		void UpdateProperty(const FName& AttrName, FProperty* Property)
		{
			// 处理基本数值类型
			if (auto* IntProperty = CastField<FIntProperty>(Property))
			{
				UpdateVTable(AttrName, IntProperty);
			}
			// 处理浮点数
			else if (auto* FloatProperty = CastField<FFloatProperty>(Property))
			{
				UpdateVTable(AttrName, FloatProperty);
			}
			// 处理字符串
			else if (auto* StrProperty = CastField<FStrProperty>(Property))
			{
				UpdateVTable(AttrName, StrProperty);
			}
			else {
				UE_LOG(Lua, Error, TEXT("Unhandle Attribute:Name:%s"), *AttrName.ToString());
			}
		}

		template <typename PropertyType>
		AttributeInfo* GenAttributeInfo(PropertyType *Property)
		{
			AttributeInfo* Attribute = new AttributeInfo;

			Attribute->Getter = [Property](UObject* Obj, lua_State* State) {
				PushValue(State, PropertyInfo<PropertyType>::GetValue(Obj, Property));
				};

			Attribute->Setter = [Property](UObject* Obj, lua_State* State) {
				using InfoType = PropertyInfo<PropertyType>;
				InfoType::SetValue(Obj, Property, 
					GetValue<InfoType::ValueType>(State, -1));
				};


			return Attribute;
		}

		AttributeInfo* GenAttributeInfo(UFunction* Function)
		{
			AttributeInfo* Info = new AttributeInfo;

			Info->Getter = [Function](UObject* Obj, lua_State* State) {
					uint8* Params = (uint8*)FMemory_Alloca(Function->ParmsSize);
					FMemory::Memzero(Params, Function->ParmsSize);

					for (TFieldIterator<FProperty> It(Function); It; ++It) {
						FProperty* Property = *It;
						// 处理基本数值类型
						if (auto* IntProperty = CastField<FIntProperty>(Property))
						{
							auto Value = GetValue<int>(State, 3);
							PropertyInfo<FIntProperty>::SetValue(Params, IntProperty, Value);
						//	UpdateVTable(AttrName, IntProperty);
						}
					}

					Obj->ProcessEvent(Function, Params);

					// free the param ????
				};

			// Info->Setter = Info->Getter;

			return Info;
		}

		void CallFunction()
		{
			
		}

		void UpdateFunction(const FName& AttrName, UFunction* Function)
		{
			AttributeInfo* Attribute = GenAttributeInfo(Function);

			Call("_lua_update_vtable_fun", Name, AttrName, Attribute);
			// UpdateVTable(AttrName, Function);
			//uint8* Params = (uint8*)FMemory_Alloca(Function->ParmsSize);
			//FMemory::Memzero(Params);

			//for (TFieldIterator<FProperty> It(Function); It; ++It) {
			//	FProperty* Property = *It;
			//}

			//Function->ProcessEvent(Function, Params);
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
			Mgr.VTables.Add(TypeId, NewVTable);
			return NewVTable;
		}

		TMap<FString, VTable*> VTables;
	};

	static void UpdateVTable(const FString& TypeId, const FName& Name)
	{
		UClass* Class = LoadObject<UClass>(nullptr, *TypeId, nullptr, LOAD_Quiet);
		if (Class == nullptr) {
			return;
		}

		VTable* Table = VTableMgr::GetVTable(TypeId);
		Table->AddAttribute(Name);
	}

	static int GetAttribute(lua_State* State)
	{
		UObject* Object = GetValue<UObject*>(State, 1);
		AttributeInfo* Attr = GetValue<AttributeInfo*>(State, 2);

		Attr->Getter(Object, State);
		return 1;
	}

	static int SetAttribute(lua_State* State)
	{
		UObject* Object = GetValue<UObject*>(State, 1);
		AttributeInfo* Attr = GetValue<AttributeInfo*>(State, 2);
		Attr->Setter(Object, State);
		return 0;
	}

	void RegisterCppLua()
	{
		lua_State* State = GetLuaState();
		RegisterCallback("update_vtable", UpdateVTable);
		lua_register(State, "_cpp_get_attr", GetAttribute);
		lua_register(State, "_cpp_set_attr", SetAttribute);
	}
}