#include <map>
#include <string>

#include "TLua.h"
#include "TLua.hpp"
#include "TLuaCppLua.hpp"
#include "TLuaTypes.hpp"
#include "TLuaProperty.hpp"

#include "TLuaRootObject.h"

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/UObjectGlobals.h"

namespace TLua
{
	FunctionContext::FunctionContext(UFunction* InFunction) 
		: Function(InFunction), Return(nullptr)
	{
		ProcessParameterProperty();
		ProcessReturnProperty();
	}

	// _cpp_object_call(Object, Context, args...)
	int FunctionContext::Call(lua_State* State, UObject* Object)
	{
		void* Parameters = (void*)FMemory_Alloca(Function->ParmsSize);
		FillParameters(Parameters, State, 3);

		Object->ProcessEvent(Function, Parameters);

		return FreeParameter(Parameters, State);
	}

	void FunctionContext::FillParameters(void* Parameters, lua_State* State, int ArgStartIndex)
	{
		FMemory::Memzero(Parameters, Function->ParmsSize);

		int LuaTop = LuaGetTop(State);
		for (int Index = 0; Index < ParameterProcessors.Num(); ++Index) {
			// _cpp_object_call_fun(self, fun_context, args...)
			PropertyProcessor* Processor = ParameterProcessors[Index];

			int LuaIndex = Index + ArgStartIndex;
			if (LuaIndex <= LuaTop) {
				Processor->FromLua(State, LuaIndex, Parameters);
			}
			else {
				Processor->Property->InitializeValue_InContainer(Parameters);
			}
		}
	}

	int FunctionContext::FreeParameter(void* Parameters, lua_State* State)
	{
		// free parameter
		for (PropertyProcessor* Processor : ParameterProcessors) {
			Processor->DestroyValue_InContainer(Parameters);
		}

		// process return
		if (Return) {
			Return->ToLua(State, Parameters);
			Return->DestroyValue_InContainer(Parameters);
			return 1;
		}

		return 0;
	}

//	private:
	void FunctionContext::ProcessParameterProperty()
	{
		for (TFieldIterator<FProperty> It(Function); It; ++It) {
			FProperty* Property = *It;

			if (Property->HasAnyPropertyFlags(CPF_ReturnParm)) {
				continue;
			}
			// add the processor
			ParameterProcessors.Add(CreatePropertyProcessor(Property));
		}
	}

	void FunctionContext::ProcessReturnProperty()
	{
		FProperty* Property = Function->GetReturnProperty();
		if (!Property) {
			return;
		}

		Return = CreatePropertyProcessor(Property);
	}

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

	int CppObjectGetAttrs(lua_State* State)
	{
		UClass* Class = (UClass*)lua_touserdata(State, 1);

		TArray<FString> Result;

		for (TFieldIterator<FProperty> It(Class); It; ++It) {
			FProperty* Property = *It;

			Result.Push(Property->GetName());
		}

		PushValue(State, Result);
		return 1;
	}

	int CppObjectGetType(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);

		lua_pushlightuserdata(State, Object->GetClass());
		return 1;
	}

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
		FName Name = GetValue<FName>(State, 2);
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
			UActorComponent* Component = NewObject<UActorComponent>(Actor, FoundClass, Name);
			Component->RegisterComponent();
			PushValue(State, Component);
			return 1;
		}

		return 0;
	}

	// _cpp_object_remove_from_parent()
	int CppObjectRemoveFromParent(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);
		Object->Rename(nullptr, nullptr, REN_DontCreateRedirectors);
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
		lua_pushinteger(State, Value);

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

	// _cpp_load_class(name) -> UClass
	static int CppLoadClass(lua_State* State)
	{
		FString Name = TypeInfo<FString>::FromLua(State, 1);

		UClass* Class = FindObject<UClass>(nullptr, *Name);
		if (!Class) {
			Class = LoadObject<UClass>(nullptr, *Name);
		}
		if (Class) {
			lua_pushlightuserdata(State, Class);
			return 1;
		}

		return 0;
	}

	// _cpp_new_object(UClass*)
	int CppNewObject(lua_State* State)
	{
		UObject* Outter = (UObject*)lua_touserdata(State, 1);
		UClass* Class = (UClass*)lua_touserdata(State, 2);

		UObject* Object = NewObject<UObject>(Outter, Class);
		if (Object) {
			lua_pushlightuserdata(State, Object);
			return 1;
		}

		return 0;
	}

	// _cpp_get_engine()
	int CppGetEngine(lua_State* State)
	{
		lua_pushlightuserdata(State, GEngine);
		return 1;
	}

	// _cpp_engine_callback(root_object, delta, fun)
	int CppEngineCallback(lua_State* State)
	{
		UTLuaRootObject* Root = (UTLuaRootObject*)lua_touserdata(State, 1);
		int Handler = Root->AddCallback(State);
		lua_pushinteger(State, Handler);
		return 1;
	}

	// _cpp_create_default_subobject(Object, Class.CameraComponent, FName)
	int CppCreateDefaultSubobject(lua_State* State)
	{
		int Top = lua_gettop(State);
		AActor* Parent = TypeInfo<AActor*>::FromLua(State, 1);
		FName Name = TypeInfo<FName>::FromLua(State, 2);
		UClass* Class = (UClass*)lua_touserdata(State, 3);

		UObject* Object = Parent->CreateDefaultSubobject(Name, UObject::StaticClass(), Class, true, false);
		if (!Object) {
			return 0;
		}

		UActorComponent* ActorComponent = Cast<UActorComponent>(Object);
		if (ActorComponent) {
			PushValue(State, ActorComponent);
			return 1;
		}

		PushValue(State, Object);
		return 1;
	}

	// _cpp_delegate_execute(Delegate, Accessor)
	int CppDelegateExecute(lua_State* State)
	{
		FScriptDelegate* Delegate = (FScriptDelegate*)lua_touserdata(State, 1);
		DelegateAccessor* Accessor = (DelegateAccessor*)lua_touserdata(State, 2);

		return Accessor->Execute(Delegate, State, 3);
	}

	// _cpp_delegate_bind(Delegate, Accessor, lua_fun)
	int CppDelegateBind(lua_State* State)
	{
		FScriptDelegate* Delegate = (FScriptDelegate*)lua_touserdata(State, 1);
		DelegateAccessor* Accessor = (DelegateAccessor*)lua_touserdata(State, 2);
		Accessor->Bind(Delegate, State, 3);

		return 0;
	}

	void RegisterCppLua()
	{
		lua_State* State = GetLuaState();

		// global use
		lua_register(State, "_cpp_load_class", CppLoadClass);
		lua_register(State, "_cpp_create_default_subobject", CppCreateDefaultSubobject);
		lua_register(State, "_cpp_new_object", CppNewObject);

		// engine
		lua_register(State, "_cpp_get_engine", CppGetEngine);
		lua_register(State, "_cpp_engine_callback", CppEngineCallback);

		// struct
		lua_register(State, "_cpp_struct_get_name", CppStructGetName);

		// object
		lua_register(State, "_cpp_object_get_name", CppObjectGetName);
		lua_register(State, "_cpp_object_get_attrs", CppObjectGetAttrs);
		lua_register(State, "_cpp_object_get_type", CppObjectGetType);
		lua_register(State, "_cpp_object_get_info", CppObjectGetInfo);
		lua_register(State, "_cpp_object_call_fun", CppObjectCallFun);
		lua_register(State, "_cpp_object_create", CppObjectCreate);
		lua_register(State, "_cpp_object_remove_from_parent", CppObjectRemoveFromParent);

		// enum
		lua_register(State, "_cpp_enum_get_type_name", CppEnumGetTypeName);
		lua_register(State, "_cpp_enum_get_type", CppEnumGetType);
		lua_register(State, "_cpp_enum_get_value", CppEnumGetValue);
		lua_register(State, "_cpp_enum_get_name", CppEnumGetName);

		// delegate
		lua_register(State, "_cpp_delegate_execute", CppDelegateExecute);
		lua_register(State, "_cpp_delegate_bind", CppDelegateBind);
	}
}