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
		virtual void SetParameter(void* Params, lua_State* State) = 0;
		virtual void DestroyValue_InContainer(void *Container) = 0;
		// push to state
		virtual void PushValue(lua_State* State, void* Parameters) = 0;
	};

	template <typename PropertyType, typename ValueType>
	class ParameterType : public ParameterBase
	{
	public:
		ParameterType(PropertyType* InProperty, int InLuaIndex)
			: Property(InProperty), LuaIndex(InLuaIndex)
		{
		}

		virtual void SetParameter(void* Parameters, lua_State* State) override
		{
			using PropertyInfoType = PropertyInfo<PropertyType>;
			PropertyInfoType::SetValue(Parameters, Property,
				GetValue<typename PropertyInfoType::ValueType>(State, LuaIndex));
		}

		virtual void DestroyValue_InContainer(void* Container) override
		{
			Property->DestroyValue_InContainer(Container);
		}

		//virtual void PushValue(lua_State* State, void* Parameters) override
		//{
		//	using PropertyInfoType = PropertyInfo<PropertyType>;

		//	TLua::PushValue(State,
		//		PropertyInfo<PropertyType>::GetValue(Parameters, Property));
		//}

		void PushValue(lua_State* State, void* Object)
		{
			// 1. get value from UObject
			// 2. push value to lua (by special type)
			ValueType Value = (ValueType)Property->GetPropertyValue_InContainer(Object);
			TLua::PushValue(State, Value);
		}

		//template <typename ObjectType>
		//int CppObjectGetObject(lua_State* State)
		//{
		//	UObject* Object = (UObject*)lua_touserdata(State, 1);
		//	FObjectProperty* Property = (FObjectProperty*)lua_touserdata(State, 2);

		//	auto Ptr = Property->GetPropertyValue_InContainer(Object);
		//	PushValue(State, Cast<ObjectType>(Ptr));

		//	return 1;
		//}

	private:
		PropertyType* Property;
		int LuaIndex;
	};

	class ParameterVisitor
	{
	public:
		ParameterVisitor(int InIndex = -1) : Index(InIndex), Parameter(nullptr)
		{
		}
		
		template <typename PropertyType> 
		inline void Visit(PropertyType* Property)
		{
			using ValueType = typename PropertyInfo<PropertyType>::ValueType;
			Parameter = new ParameterType<PropertyType, ValueType>(Property, Index);
		}

		void Visit(FObjectProperty* Property, UActorComponent*)
		{
			Parameter = new ParameterType<FObjectProperty, UActorComponent*>(Property, Index);
		}

		void Visit(FObjectProperty* Property, AActor*)
		{
			Parameter = new ParameterType<FObjectProperty, AActor*>(Property, Index);
		}

	public:
		int Index;
		ParameterBase* Parameter;
	};

	class FunctionContext
	{
	public:
		FunctionContext(UFunction* InFunction) : Function(InFunction), Return(nullptr)
		{
			ProcessParameter();
			ProcessReturn();
		}

		int Call(lua_State* State, UObject* Object)
		{
			// check the parameter size
			int Top = lua_gettop(State);
			if (Top != Parameters.Num() + 2) {	// self, context, args...
				UE_LOG(Lua, Error, TEXT("invalid argument number, %s, %d"), *Function->GetName(), Function->NumParms);
				return 0;
			}

			void* ParameterMemory = (void*)FMemory_Alloca(Function->ParmsSize);
			FMemory::Memzero(ParameterMemory, Function->ParmsSize);

			// set the parameter
			for (ParameterBase* Parameter : Parameters) {
				Parameter->SetParameter(ParameterMemory, State);
			}
				
			Object->ProcessEvent(Function, ParameterMemory);

			// free the parameter
			for (ParameterBase* Parameter : Parameters) {
				Parameter->DestroyValue_InContainer(ParameterMemory);
			}

			if (Return) {
				// push to lua 
				Return->PushValue(State, ParameterMemory);
				// release the memory
				Return->DestroyValue_InContainer(ParameterMemory);
				return 1;
			}

			return 0;
		}
	private:
		void ProcessParameter()
		{
			int Index = 2;	// start from 2, // lua_call(self, context, args...)
			for (TFieldIterator<FProperty> It(Function); It; ++It) {
				FProperty* Property = *It;
				Index += 1;

				if (Property->HasAnyPropertyFlags(CPF_ReturnParm)) {
					continue;
				}

				ParameterVisitor Visitor(Index);
				DispatchProperty(Property, Visitor);

				Parameters.Add(Visitor.Parameter);
			}
		}

		void ProcessReturn()
		{
			FProperty* Property = Function->GetReturnProperty();
			if (!Property) {
				return;
			}

			ParameterVisitor Visitor;
			DispatchProperty(Property, Visitor);
			Return = Visitor.Parameter;
		}

	private:
		UFunction* Function;
		TArray<ParameterBase*> Parameters;
		ParameterBase* Return;
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

	template <typename ObjectType>
	int CppObjectGetObject(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);
		FObjectProperty* Property = (FObjectProperty*)lua_touserdata(State, 2);

		auto Ptr = Property->GetPropertyValue_InContainer(Object);
		PushValue(State, Cast<ObjectType>(Ptr));

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

	template <typename ObjectType>
	int CppObjectSetObject(lua_State* State)
	{
		UObject* Object = (UObject*)lua_touserdata(State, 1);
		FObjectProperty* Property = (FObjectProperty*)lua_touserdata(State, 2);

		ObjectType* Value = GetValue<ObjectType*>(State, 3);

		Property->SetValue_InContainer(Object, Value);

		return 0;
	}

	struct ObjectPropertyVisitor
	{
		ObjectPropertyVisitor(lua_State* InState) : State(InState)
		{
		}

		template <typename PropertyType>
		void Visit(PropertyType* Property, void* = nullptr)
		{
			lua_pushlightuserdata(State, Property);
			lua_pushcfunction(State, &CppObjectGetAttr<PropertyType>);
			lua_pushcfunction(State, &CppObjectSetAttr<PropertyType>);
		}

		void Visit(FObjectProperty* Property, UActorComponent* = nullptr)
		{
			lua_pushlightuserdata(State, Property);
			lua_pushcfunction(State, &CppObjectGetObject<UActorComponent>);
			lua_pushcfunction(State, &CppObjectSetObject<UActorComponent>);
		}

		void Visit(FObjectProperty* Property, AActor* = nullptr)
		{
			lua_pushlightuserdata(State, Property);
			lua_pushcfunction(State, &CppObjectGetObject<AActor>);
			lua_pushcfunction(State, &CppObjectSetObject<AActor>);
		}

		lua_State* State;
	};

	int CppObjectCallFun(lua_State* State)
	{
		UClass* Object = (UClass*)lua_touserdata(State, 1);
		FunctionContext* Function = (FunctionContext*)lua_touserdata(State, 2);

		return Function->Call(State, Object);
	}

	int CppObjectGetInfo(lua_State* State)
	{
		UClass* Class = (UClass*)lua_touserdata(State, 1);
		FName Name(lua_tostring(State, 2));

		FProperty* Property = Class->FindPropertyByName(Name);
		if (Property) {
			ObjectPropertyVisitor Visitor(State);
			DispatchProperty(Property, Visitor);
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
		return 0;
	}

	void RegisterCppLua()
	{
		lua_State* State = GetLuaState();

		lua_register(State, "_cpp_struct_get_name", CppStructGetName);

		lua_register(State, "_cpp_object_get_name", CppObjectGetName);
		lua_register(State, "_cpp_object_get_info", CppObjectGetInfo);
		lua_register(State, "_cpp_object_call_fun", CppObjectCallFun);
	}
}