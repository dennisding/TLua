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
		virtual void DestroyValue_InContainer(void *Container) = 0;
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

		virtual void DestroyValue_InContainer(void* Container)
		{
			Property->DestroyValue_InContainer(Container);
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
			Call("_lua_update_vtable_attr", Name, AttrName, (void*)Attribute);
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

					// set the parameter
					for (ParameterBase* Parameter : Info->Parameters) {
						Parameter->SetParameter(Params, State);
					}
					
					Obj->ProcessEvent(Function, Params);

					// free the parameter
					for (ParameterBase* Parameter : Info->Parameters) {
						Parameter->DestroyValue_InContainer(Params);
					}
				};
			// Info->Setter = Info->Getter;
			return Info;
		}

		void UpdateFunction(const FName& AttrName, UFunction* Function)
		{
			AttributeInfo* Attribute = GenAttributeInfo(Function);

			Call("_lua_update_vtable_fun", Name, AttrName, (void*)Attribute);
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

	static int UpdateVTable(lua_State* State)
	{
		FString TypeId = GetValue<FString>(State, -2);
		FName Name = GetValue<FName>(State, -1);

		UClass* Class = LoadObject<UClass>(nullptr, *TypeId, nullptr, LOAD_Quiet);
		if (Class == nullptr) {
			return 0;
		}

		VTable* Table = VTableMgr::GetVTable(TypeId);
		Table->AddAttribute(Name);

		return 0;
	}

	static int GetAttribute(lua_State* State)
	{
		UObject* Object = (UObject*)GetValue<void*>(State, 1);
		AttributeInfo* Attr = (AttributeInfo*)GetValue<void*>(State, 2);

		Attr->Getter(Object, State);
		return 1;
	}

	static int SetAttribute(lua_State* State)
	{
		UObject* Object = (UObject*)GetValue<void*>(State, 1);
		AttributeInfo* Attr = (AttributeInfo*)GetValue<void*>(State, 2);
		Attr->Setter(Object, State);
		return 0;
	}

	static int CppStructGetName(lua_State* State)
	{
		UStruct* Struct = (UStruct*)GetValue<void*>(State, 1);

		FString Name = Struct->GetName();
		FTCHARToUTF8 Convert(Name);
		lua_pushlstring(State, (const char*)Convert.Get(), Convert.Length());

		return 1;
	}

	//static int CppStructGetAttr(lua_State* State)
	//{
	//	UStruct* Struct = GetValue<UStruct*>(State, 1);
	//	const char* LuaName = lua_tostring(State, 2);
	//	FName Name(LuaName);

	//	FProperty* Property = Struct->FindPropertyByName(Name);
	//	if (Property) {
	//		// update property
	//		return 1;
	//	}

	//	UFunction* Function = Struct->FindFunction(Name);
	//	if (Function) {
	//		// update function
	//		return 1;
	//	}

	//	return 0;
	//}

	int CppObjectGetName(lua_State* State)
	{
		UClass* Object = (UClass*)lua_touserdata(State, 1);

		FString Name = Object->GetName();
		FTCHARToUTF8 Convert(Name);
		lua_pushlstring(State, (const char*)Convert.Get(), Convert.Length());

		return 1;
	}

	//int CppObjectGetAttr(lua_State* State)
	//{
	//	return 1;
	//}

	template <typename PropertyType>
	int CppObjectGetAttr(lua_State* State)
	{
		using ValueType = typename PropertyInfo<PropertyType>::ValueType;

		UObject* Object = (UObject*)lua_touserdata(State, 1);
		PropertyType* Property = (PropertyType*)lua_touserdata(State, 2);

		PushValue(State, 
			PropertyInfo<PropertyType>::GetValue(Object, Property));

		return 1;
	}

	template <typename PropertyType>
	int CppObjectSetAttr(lua_State* State)
	{
		using ValueType = typename PropertyInfo<PropertyType>::ValueType;
		
		UObject* Object = (UObject*)lua_touserdata(State, 1);
		PropertyType* Property = (PropertyType*)lua_touserdata(State, 2);
		ValueType Value = GetValue<ValueType>(State, 3);

		PropertyInfo<PropertyType>::SetValue(Object, Property, Value);

		return 0;
	}

	int CppObjectCallFun(lua_State* State)
	{
		return 0;
	}

	struct ObjectPropertyVisitor
	{
		ObjectPropertyVisitor(lua_State* InState) : State(InState)
		{
		}

		template <typename PropertyType>
		void Visit(PropertyType* Property)
		{
			lua_pushlightuserdata(State, Property);
			lua_pushcfunction(State, &CppObjectGetAttr<PropertyType>);
			lua_pushcfunction(State, &CppObjectSetAttr<PropertyType>);
		}

		lua_State* State;
	};

	int CppObjectGetInfo(lua_State* State)
	{
		UClass* Object = (UClass*)lua_touserdata(State, 1);
		FName Name(lua_tostring(State, 2));

		FProperty* Property = Object->FindPropertyByName(Name);
		if (Property) {
			ObjectPropertyVisitor Visitor(State);
			DispatchProperty(Property, Visitor);
			return 3;
		}
		UFunction* Function = Object->FindFunctionByName(Name);
		if (Function) {
			return 1;
		}
		return 0;
	}

	void RegisterCppLua()
	{
		lua_State* State = GetLuaState();
		lua_register(State, "_cpp_update_vtable", UpdateVTable);
		lua_register(State, "_cpp_get_attr", GetAttribute);
		lua_register(State, "_cpp_set_attr", SetAttribute);

		lua_register(State, "_cpp_struct_get_name", CppStructGetName);
//		lua_register(State, "_cpp_struct_get_attr", CppStructGetAttr);

		lua_register(State, "_cpp_object_get_name", CppObjectGetName);
		lua_register(State, "_cpp_object_get_info", CppObjectGetInfo);
	}
}