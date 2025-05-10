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
	class ParameterBase
	{
	public:
		virtual void SetParameter(uint8* Params, lua_State* State) = 0;
	};

	struct AttributeInfo
	{
	public:
		std::function<void(UObject*, lua_State*)> Setter;
		std::function<void(UObject*, lua_State*)> Getter;
		TArray<ParameterBase*> Parameters;
	};

	template <typename PropertyType>
	class ParameterType : public ParameterBase
	{
	public:
		ParameterType(PropertyType* InProperty, int InLuaIndex)
			: Property(InProperty), LuaIndex(InLuaIndex)
		{
		}

		virtual void SetParameter(uint8* Params, lua_State* State) override
		{
			using PropertyInfoType = PropertyInfo<PropertyType>;
			PropertyInfoType::SetValue(Params, Property,
				GetValue<typename PropertyInfoType::ValueType>(State, LuaIndex));
		}

	private:
		PropertyType* Property;
		int LuaIndex;
	};

	class ParameterVisitor
	{
	public:
		ParameterVisitor(int InIndex, AttributeInfo* InInfo)
			: Index(InIndex), Info(InInfo)
		{

		}
		
		template <typename PropertyType> 
		inline void Visit(PropertyType* Property)
		{
			Info->Parameters.Add(new ParameterType<PropertyType>(Property, Index));
		}

	private:
		int Index;
		AttributeInfo* Info;
	};

	class VTable;

	class AttributeVisitor
	{
	public:
		AttributeVisitor(VTable* InTable, const FString& InTypeName, const FName& InAttrName)
			: Table(InTable), TypeName(InTypeName), AttrName(InAttrName)
		{
		
		}

		template <typename PropertyType>
		void Visit(PropertyType* Property);

	private:
		VTable* Table;
		FString TypeName;
		FName AttrName;
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

		template <typename Type>
		void UpdateVTable(const FName& AttrName, Type* Property)
		{
			AttributeInfo* Attribute = GenAttributeInfo(Property);
			Call("_lua_update_vtable_attr", Name, AttrName, Attribute);
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

			FProperty* Property = Class->FindPropertyByName(AttrName);
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
			Call("_lua_update_vtable_attr", Name, AttrName, 0);
		}

		void UpdateProperty(const FName& AttrName, FProperty* Property)
		{
			AttributeVisitor Visitor(this, Name, AttrName);
			DispatchProperty(Property, Visitor);
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

			// dispatch the property
			int ParamIndex = 2;				// getter(UObject*, AttributeInfo*,Args...)
			for (TFieldIterator<FProperty> It(Function); It; ++It) {
				FProperty* Property = *It;
				ParamIndex += 1;

				ParameterVisitor Visitor(ParamIndex, Info);
				DispatchProperty(Property, Visitor);
			}

			Info->Getter = [Function, Info](UObject* Obj, lua_State* State) {
					uint8* Params = (uint8*)FMemory_Alloca(Function->ParmsSize);
					FMemory::Memzero(Params, Function->ParmsSize);

					for (ParameterBase* Parameter : Info->Parameters) {
						Parameter->SetParameter(Params, State);
					}
					
					Obj->ProcessEvent(Function, Params);
					// free the param ????
				};
			// Info->Setter = Info->Getter;
			return Info;
		}

		void UpdateFunction(const FName& AttrName, UFunction* Function)
		{
			AttributeInfo* Attribute = GenAttributeInfo(Function);

			Call("_lua_update_vtable_fun", Name, AttrName, Attribute);
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

	template <typename PropertyType>
	void AttributeVisitor::Visit(PropertyType* Property)
	{
		Table->UpdateVTable(AttrName, Property);
	}

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