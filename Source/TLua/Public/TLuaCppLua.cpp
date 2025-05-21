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

	private:
		void ProcessParameterProperty()
		{
			for (TFieldIterator<FProperty> It(Function); It; ++It) {
				FProperty* Property = *It;

				if (Property->HasAnyPropertyFlags(CPF_ReturnParm)) {
					continue;
				}

				ParameterProcessors.Add(CreatePropertyProcessor(Property));
			}
		}

		void ProcessReturnProperty()
		{
			FProperty* Property = Function->GetReturnProperty();
			if (!Property) {
				return;
			}

			Return = CreatePropertyProcessor(Property);
		}

	private:
		UFunction* Function;
		PropertyProcessor* Return;
		ProcessorArray ParameterProcessors;
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