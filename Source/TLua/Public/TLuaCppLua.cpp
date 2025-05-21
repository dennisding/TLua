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
//	class ParameterBase
//	{
//	public:
//		virtual bool SetParameter(void* Params, lua_State* State, int Index) = 0;
//		virtual void DestroyValue_InContainer(void *Container) = 0;
//		// push to state
//		virtual void PushValue(lua_State* State, void* Parameters) = 0;
//	};
//
//	template <typename PropertyType, typename ValueType>
//	class ParameterType : public ParameterBase
//	{
//	public:
//		ParameterType(PropertyType* InProperty)
//			: Property(InProperty),
//			HasDefaultValue(false), DefaultValue(ValueType()), IsSetted(false)
//		{
//			if (Property->HasAnyPropertyFlags(CPF_Parm) &&
//				!Property->HasAnyPropertyFlags(CPF_OutParm))
//			{
//				if (Property->HasMetaData("CPP_DefaultValue")) {
//					HasDefaultValue = true;
//					DefaultValue = (ValueType)PropertyInfo<PropertyType>::GetDefault(Property);
//				}
//			}
//		}
//
//		virtual ~ParameterType() {}
//
//		virtual bool SetParameter(void* Parameters, lua_State* State, int Index) override
//		{
//			using PropertyInfoType = PropertyInfo<PropertyType>;
//
//			if (LuaGetTop(State) < Index) { // not enough parameter
//				if (HasDefaultValue) {
//					PropertyInfoType::SetValue(Parameters, Property, DefaultValue);
//					IsSetted = true;
//					return true;
//				}
//				IsSetted = false;
//				return false;
//			}
//
//			PropertyInfoType::SetValue(Parameters, Property,
//				GetValue<typename PropertyInfoType::ValueType>(State, Index));
//
//			IsSetted = true;
//			return true;
//		}
//
//		virtual void DestroyValue_InContainer(void* Container) override
//		{
//			if (IsSetted) {
//				Property->DestroyValue_InContainer(Container);
//				IsSetted = true;
//			}
//		}
//
//		void PushValue(lua_State* State, void* Object) override
//		{
//			// 1. get value from UObject
//			// 2. push value to lua (by special type)
//			//ValueType Value = (ValueType)Property->GetPropertyValue_InContainer(Object);
//			ValueType Value = (ValueType)PropertyInfo<PropertyType>::GetValue(Object, Property);
//			TLua::PushValue(State, Value);
//		}
//
//	private:
//		PropertyType* Property;
//		bool HasDefaultValue;
//		ValueType DefaultValue;
//		bool IsSetted;
//	};
//
//	// to do: process the default value
//	class ParameterStructType : public ParameterBase
//	{
//	public:
//		ParameterStructType(FStructProperty* InProperty) : Property(InProperty)
//		{
//			Properties = UStructMgr::Get(Property->Struct);
//		}
//
//		virtual bool SetParameter(void* Parameters, lua_State* State, int Index) override
//		{
//			for (PropertyBase* ChildProperty : *Properties) {
//				ChildProperty->GetLuaField(State, Index, Parameters);
//			}
//
//			return true;
//		}
//		virtual void DestroyValue_InContainer(void* Container) override
//		{
//			Property->DestroyValue_InContainer(Container);
//		}
//		// push to state
//		virtual void PushValue(lua_State* State, void* Parameters) override
//		{
//			// LuaPushNil(State);
//			void* ValuePtr = Property->ContainerPtrToValuePtr<void*>(Parameters);
//
//			LuaNewTable(State);
//			TypeInfo<void*>::ToLua(State, Property->Struct);
//			LuaSetField(State, -2, "_ct");
////			SetTableByName(State, -1, "_ct", (void*)Property->Struct);
//			// iter the property
//			for (PropertyBase* ChildProperty : *Properties) {
////				Property->SetField(State, -1, &Value);
//				ChildProperty->SetLuaField(State, -1, ValuePtr);
//			}
//
//			LuaGetGlobal(State, "_ls");
//			LuaSetMetatable(State, -2);
//		}
//	private:
//		FStructProperty* Property;
//		PropertyArray* Properties;
//	};
//
//	class ParameterVisitor
//	{
//	public:
//		ParameterVisitor() : Parameter(nullptr)
//		{
//		}
//		
//		template <typename PropertyType> 
//		inline void Visit(PropertyType* Property)
//		{
//			using ValueType = typename PropertyInfo<PropertyType>::ValueType;
//			Parameter = new ParameterType<PropertyType, ValueType>(Property);
//		}
//
//		void Visit(FStructProperty* Property)
//		{
//		}
//
//		void Visit(FObjectProperty* Property, UActorComponent*)
//		{
//			Parameter = new ParameterType<FObjectProperty, UActorComponent*>(Property);
//		}
//
//		void Visit(FObjectProperty* Property, AActor*)
//		{
//			Parameter = new ParameterType<FObjectProperty, AActor*>(Property);
//		}
//
//	public:
//		ParameterBase* Parameter;
//	};

	class FunctionContext
	{
	public:
		FunctionContext(UFunction* InFunction) : Function(InFunction), Return(nullptr)
		{
			ProcessParameterProperty();
			ProcessReturnProperty();
		}

		// _cpp_object_call(Object, Context, args...)
		int Call(lua_State* State, UObject* Object)
		{
			// setup parameter
			void* ParameterMemory = (void*)FMemory_Alloca(Function->ParmsSize);
			FMemory::Memzero(ParameterMemory, Function->ParmsSize);
			for (int Index = 0; Index < ParameterProcessors.Num(); ++Index) {
				// _cpp_object_call_fun(self, fun_context, args...)
				int LuaIndex = Index + 3;
				PropertyProcessor* Processor = ParameterProcessors[Index];
				Processor->FromLua(State, LuaIndex, ParameterMemory);
			}

			// call the function
			Object->ProcessEvent(Function, ParameterMemory);

			// free parameter
			for (PropertyProcessor* Processor : ParameterProcessors) {
				Processor->DestroyValue_InContainer(ParameterMemory);
			}

			// process return
			if (Return) {
				Return->ToLua(State, ParameterMemory);
				Return->DestroyValue_InContainer(ParameterMemory);
				return 1;
			}

			return 0;
		}

		//int Call(lua_State* State, UObject* Object)
		//{
		//	void* ParameterMemory = (void*)FMemory_Alloca(Function->ParmsSize);
		//	FMemory::Memzero(ParameterMemory, Function->ParmsSize);

		//	// set the parameter
		//	bool ValidParameter = true;
		//	// for (ParameterBase* Parameter : Parameters) {
		//	// start from 3, _cpp_boject_call_fun(self, fun_context, args...)
		//	for (int Index = 3; Index <= Parameters.Num(); ++Index) {
		//		ParameterBase* Parameter = Parameters[Index];
		//		ValidParameter = 
		//			ValidParameter && Parameter->SetParameter(ParameterMemory, State, Index);
		//	}

		//	if (!ValidParameter) {
		//		// free the parameter
		//		for (ParameterBase* Parameter : Parameters) {
		//			Parameter->DestroyValue_InContainer(ParameterMemory);
		//			UE_LOG(Lua, Error, TEXT("Invalid Set Parameter"));
		//		}
		//		return 0;
		//	}

		//		
		//	Object->ProcessEvent(Function, ParameterMemory);

		//	// free the parameter
		//	for (ParameterBase* Parameter : Parameters) {
		//		Parameter->DestroyValue_InContainer(ParameterMemory);
		//	}

		//	if (Return) {
		//		// push to lua 
		//		Return->PushValue(State, ParameterMemory);
		//		// release the memory
		//		Return->DestroyValue_InContainer(ParameterMemory);
		//		return 1;
		//	}

		//	return 0;
		//}
	private:
		void ProcessParameterProperty()
		{
			// start from 2, // lua_call(self, context, args...)
			for (TFieldIterator<FProperty> It(Function); It; ++It) {
				FProperty* Property = *It;

				if (Property->HasAnyPropertyFlags(CPF_ReturnParm)) {
					continue;
				}

				ParameterProcessors.Add(CreatePropertyProcessor(Property));
			}

			//for (TFieldIterator<FProperty> It(Function); It; ++It) {
			//	FProperty* Property = *It;
			//
			//	if (Property->HasAnyPropertyFlags(CPF_ReturnParm)) {
			//		continue;
			//	}

			//	ParameterVisitor Visitor;
			//	DispatchProperty(Property, Visitor);

			//	Parameters.Add(Visitor.Parameter);
			//}
		}

		void ProcessReturnProperty()
		{
			FProperty* Property = Function->GetReturnProperty();
			if (!Property) {
				return;
			}

			Return = CreatePropertyProcessor(Property);

			//ParameterVisitor Visitor;
			//DispatchProperty(Property, Visitor);
			//Return = Visitor.Parameter;
		}

	private:
		UFunction* Function;
		PropertyProcessor* Return;
		ProcessorArray ParameterProcessors;
		//TArray<ParameterBase*> Parameters;
		//ParameterBase* Return;
	};

	static int CppStructGetName(lua_State* State)
	{
		UStruct* Struct = (UStruct*)GetValue<void*>(State, 1);

		FString Name = Struct->GetName();
		FTCHARToUTF8 Convert(Name);
		lua_pushlstring(State, (const char*)Convert.Get(), Convert.Length());

		return 1;
	}

	int CppObjectGetName(lua_State* State)
	{
		UClass* Object = (UClass*)lua_touserdata(State, 1);

		FString Name = Object->GetName();
		FTCHARToUTF8 Convert(Name);
		lua_pushlstring(State, (const char*)Convert.Get(), Convert.Length());

		return 1;
	}

	int CppObjectGetType(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);

		lua_pushlightuserdata(State, Object->GetClass());
		return 1;
	}

	//template <typename PropertyType>
	//int CppObjectGetAttr(lua_State* State)
	//{
	//	using ValueType = typename PropertyInfo<PropertyType>::ValueType;

	//	UObject* Object = (UObject*)lua_touserdata(State, 1);
	//	PropertyType* Property = (PropertyType*)lua_touserdata(State, 2);

	//	PushValue(State, 
	//		PropertyInfo<PropertyType>::GetValue(Object, Property));

	//	return 1;
	//}

	//template <typename ObjectType>
	//int CppObjectGetObject(lua_State* State)
	//{
	//	UObject* Object = (UObject*)lua_touserdata(State, 1);
	//	FObjectProperty* Property = (FObjectProperty*)lua_touserdata(State, 2);

	//	auto Ptr = Property->GetPropertyValue_InContainer(Object);
	//	PushValue(State, Cast<ObjectType>(Ptr));

	//	return 1;
	//}

	//template <typename ObjectType>
	//int CppObjectSetObject(lua_State* State)
	//{
	//	UObject* Object = (UObject*)lua_touserdata(State, 1);
	//	FObjectProperty* Property = (FObjectProperty*)lua_touserdata(State, 2);

	//	ObjectType* Value = GetValue<ObjectType*>(State, 3);

	//	Property->SetValue_InContainer(Object, Value);

	//	return 0;
	//}

	//// _call(self, property)
	//int CppObjectGetStruct(lua_State* State)
	//{
	//	UObject* Self = (UObject*)lua_touserdata(State, 1);
	//	FStructProperty* Property = (FStructProperty*)lua_touserdata(State, 1);
	//	return 0;
	//}

	//// _call(self, property, value)
	//int CppObjectSetStruct(lua_State* State)
	//{
	//	return 0;
	//}


	//template <typename PropertyType>
//int CppObjectGetAttr(lua_State* State)
//{
//	using ValueType = typename PropertyInfo<PropertyType>::ValueType;

//	UObject* Object = (UObject*)lua_touserdata(State, 1);
//	PropertyType* Property = (PropertyType*)lua_touserdata(State, 2);

//	PushValue(State, 
//		PropertyInfo<PropertyType>::GetValue(Object, Property));

//	return 1;
//}

		//template <typename PropertyType>
	//int CppObjectSetAttr(lua_State* State)
	//{
	//	using ValueType = typename PropertyInfo<PropertyType>::ValueType;
	//	
	//	UObject* Object = (UObject*)lua_touserdata(State, 1);
	//	PropertyType* Property = (PropertyType*)lua_touserdata(State, 2);
	//	ValueType Value = GetValue<ValueType>(State, 3);

	//	PropertyInfo<PropertyType>::SetValue(Object, Property, Value);

	//	return 0;
	//}

	// _cpp_object_get_attr(self, property)
	int CppObjectGetAttr(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);
		PropertyProcessor* Processor = (PropertyProcessor*)lua_touserdata(State, 2);

		Processor->ToLua(State, Object);

		return 1;
	}

	// _cpp_object_set_attr(self, property, value)
	int CppObjectSetAttr(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);
		PropertyProcessor* Processor = (PropertyProcessor*)lua_touserdata(State, 2);
		Processor->FromLua(State, 3, Object);

		return 0;
	}

	//struct ObjectPropertyVisitor
	//{
	//	ObjectPropertyVisitor(lua_State* InState) : State(InState)
	//	{
	//	}

	//	//template <typename PropertyType>
	//	//void Visit(PropertyType* Property)
	//	//{
	//	//	lua_pushlightuserdata(State, Property);
	//	//	lua_pushcfunction(State, &CppObjectGetAttr<PropertyType>);
	//	//	lua_pushcfunction(State, &CppObjectSetAttr<PropertyType>);
	//	//}


	//	//void Visit(FObjectProperty* Property)
	//	//{
	//	//	UClass* Class = Property->PropertyClass;
	//	//	if (Class->IsChildOf<UActorComponent>()) {
	//	//		lua_pushlightuserdata(State, Property);
	//	//		lua_pushcfunction(State, &CppObjectGetObject<UActorComponent>);
	//	//		lua_pushcfunction(State, &CppObjectSetObject<UActorComponent>);
	//	//	}
	//	//	else if (Class->IsChildOf<AActor>()) {
	//	//		lua_pushlightuserdata(State, Property);
	//	//		lua_pushcfunction(State, &CppObjectGetObject<AActor>);
	//	//		lua_pushcfunction(State, &CppObjectSetObject<AActor>);
	//	//	}
	//	//	else {
	//	//		lua_pushlightuserdata(State, Property);
	//	//		lua_pushcfunction(State, &CppObjectGetAttr<FObjectProperty>);
	//	//		lua_pushcfunction(State, &CppObjectSetAttr<FObjectProperty>);
	//	//	}
	//	//}

	//	//void Visit(FStructProperty* Property)
	//	//{
	//	//	lua_pushlightuserdata(State, Property);
	//	//	lua_pushcfunction(State, &CppObjectGetStruct);
	//	//	lua_pushcfunction(State, &CppObjectSetStruct);
	//	//}

	//	////void Visit(FObjectProperty* Property, UActorComponent* = nullptr)
	//	////{
	//	////	
	//	////}

	//	//void Visit(FObjectProperty* Property, AActor* = nullptr)
	//	//{
	//	//	lua_pushlightuserdata(State, Property);
	//	//	lua_pushcfunction(State, &CppObjectGetObject<AActor>);
	//	//	lua_pushcfunction(State, &CppObjectSetObject<AActor>);
	//	//}

	//private:
	//	lua_State* State;
	//};

	int CppObjectCallFun(lua_State* State)
	{
		UClass* Object = (UClass*)lua_touserdata(State, 1);
		FunctionContext* Function = (FunctionContext*)lua_touserdata(State, 2);

		return Function->Call(State, Object);
	}

	static void SetupPropertyInfo(lua_State* State, FProperty* Property)
	{
		PropertyProcessor* Processor = CreatePropertyProcessor(Property);
		lua_pushlightuserdata(State, Processor);
		lua_pushcfunction(State, CppObjectGetAttr);
		lua_pushcfunction(State, CppObjectSetAttr);
	}

	int CppObjectGetInfo(lua_State* State)
	{
		UClass* Class = (UClass*)lua_touserdata(State, 1);
		FName Name(lua_tostring(State, 2));

		FProperty* Property = Class->FindPropertyByName(Name);
		if (Property) {
			//ObjectPropertyVisitor Visitor(State);
			//DispatchProperty(Property, Visitor);
			SetupPropertyInfo(State, Property);
			return 3; // property, getter, setter
		}

		UFunction* Function = Class->FindFunctionByName(Name);
		if (Function) {
			lua_pushnil(State);
			// every UFunction have only one context at most
			// so let the memory free.
			FunctionContext* Context = new FunctionContext(Function);
			lua_pushlightuserdata(State, Context);
			return 2; // is_property, FunctionContext
		}
		return 0; // nil, nil, nil
	}

	// _lua_object_create(parent, name, type)
	int CppObjectCreate(lua_State* State)
	{
		AActor* Actor = GetValue<AActor*>(State, 1);
		FString Name = GetValue<FString>(State, 2);
		FString Type = GetValue<FString>(State, 3);
		UClass* FoundClass = FindObject<UClass>(nullptr, *Type);
		if (!FoundClass)
		{
			FoundClass = LoadObject<UClass>(nullptr, *Type);
		}

		if (!FoundClass) {
			return 0;	// return nil
		}

		// create the component
		if (FoundClass->IsChildOf(UActorComponent::StaticClass())) {
			UActorComponent* Component = NewObject<UActorComponent>(Actor, FoundClass);
			Component->RegisterComponent();
			PushValue(State, Component);
			return 1;
		}


		return 0;
	}

	// _cpp_enum_get_type_name(ctype)
	static int CppEnumGetTypeName(lua_State* State)
	{
		UEnum* Type = (UEnum*)lua_touserdata(State, 1);

		PushValue(State, TCHAR_TO_ANSI(*Type->GetName()));

		return 1;
	}

	// _cpp_enum_get_type(name)
	static int CppEnumGetType(lua_State* State)
	{
		const char* AnsiName = (const char*)lua_tostring(State, 1);

		FString Name(AnsiName);
		UEnum* Type = FindObject<UEnum>(ANY_PACKAGE, *Name);

		if (Type) {
			lua_pushlightuserdata(State, Type);
		}
		else {
			lua_pushnil(State);
		}

		return 1;
	}

	// _cpp_enum_get_value(ctype, name)
	static int CppEnumGetValue(lua_State* State)
	{
		UEnum* Type = (UEnum*)lua_touserdata(State, 1);
		const char* AnsiName = (const char*)lua_tostring(State, 2);

		FName Name(AnsiName);
		int64 Value = Type->GetValueByName(Name);
		lua_pushnumber(State, Value);

		return 1;
	}

	// _cpp_enum_get_name(ctype, value)
	static int CppEnumGetName(lua_State* State)
	{
		UEnum* Type = (UEnum*)lua_touserdata(State, 1);
		int Value = lua_tointeger(State, 2);

		FString Name = Type->GetNameStringByValue(Value);
		// convert to ansi
		PushValue(State, TCHAR_TO_ANSI(*Name));
		return 1;
	}

	void RegisterCppLua()
	{
		lua_State* State = GetLuaState();

		lua_register(State, "_cpp_struct_get_name", CppStructGetName);

		lua_register(State, "_cpp_object_get_name", CppObjectGetName);
		lua_register(State, "_cpp_object_get_type", CppObjectGetType);
		lua_register(State, "_cpp_object_get_info", CppObjectGetInfo);
		lua_register(State, "_cpp_object_call_fun", CppObjectCallFun);
		lua_register(State, "_cpp_object_create", CppObjectCreate);

		// enum
		lua_register(State, "_cpp_enum_get_type_name", CppEnumGetTypeName);
		lua_register(State, "_cpp_enum_get_type", CppEnumGetType);
		lua_register(State, "_cpp_enum_get_value", CppEnumGetValue);
		lua_register(State, "_cpp_enum_get_name", CppEnumGetName);
	}
}