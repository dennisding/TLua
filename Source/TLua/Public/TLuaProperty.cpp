
#include "TLuaProperty.hpp"

#include "TLuaCppLua.hpp"
#include "TLuaRootObject.h"

namespace TLua
{
	ProcessorVisitor::ProcessorVisitor() : Result(nullptr)
	{

	}

	void ProcessorVisitor::Visit(FObjectProperty* Property)
	{
		UClass* Class = Property->PropertyClass;
		if (Class->IsChildOf(UActorComponent::StaticClass()))
		{
			Result = new Processor<FObjectProperty, UActorComponent*>(Property);
		}
		else if (Class->IsChildOf(AActor::StaticClass()))
		{
			Result = new Processor<FObjectProperty, AActor*>(Property);
		}
		else if (Class->IsChildOf(UDataAsset::StaticClass()))
		{
			Result = new Processor<FObjectProperty, UObject*>(Property);
		}
		else 
		{
			// normal FObjectProperty
			Result = new Processor<FObjectProperty, UObject*>(Property);
		}
	}

	void ProcessorVisitor::Visit(FArrayProperty* Property)
	{
		Result = new Processor<FArrayProperty, void>(Property);
	}

	void ProcessorVisitor::Visit(FDelegateProperty* Property)
	{
		Result = new TDelegateProcessor<FDelegateProperty>(Property);
	}

	void ProcessorVisitor::Visit(FMulticastDelegateProperty* Property)
	{
		Result = new TDelegateProcessor<FMulticastDelegateProperty>(Property);
	}

	class SingleDelegateAccessor : public DelegateAccessor
	{
	public:
		SingleDelegateAccessor(FDelegateProperty* InProperty)
			: Function(InProperty->SignatureFunction), Property(InProperty)
		{
		}

		virtual int Execute(void* Self, lua_State* State, int ArgStartIndex) override
		{
			FScriptDelegate* Delegate = (FScriptDelegate*)Self;

			void* Parameters = (void*)FMemory_Alloca(Property->SignatureFunction->ParmsSize);
			Function.FillParameters(Parameters, State, ArgStartIndex);

			Delegate->ProcessDelegate<UObject>(Parameters);

			return Function.FreeParameter(Parameters, State);
		}

		virtual void Bind(void* Self, lua_State* State, int AbsIndex) override
		{
			FScriptDelegate* Delegate = (FScriptDelegate*)Self;

			UTLuaCallback* Object = NewObject<UTLuaCallback>();
			Object->Bind(AbsIndex);
			Delegate->BindUFunction(Object, TEXT("Callback"));
		}

	private:
		FunctionContext Function;
		FDelegateProperty* Property;
	};

	class MulticastDelegateAccessor : public DelegateAccessor
	{
	public:
		MulticastDelegateAccessor(FMulticastDelegateProperty* InProperty)
			: Function(InProperty->SignatureFunction), Property(InProperty)
		{
		}

		virtual int Execute(void* Self, lua_State* State, int ArgStartIndex) override
		{
			FMulticastScriptDelegate* Delegate = (FMulticastScriptDelegate*)Self;

			void* Parameters = (void*)FMemory_Alloca(Property->SignatureFunction->ParmsSize);
			Function.FillParameters(Parameters, State, ArgStartIndex);

			Delegate->ProcessMulticastDelegate<UObject>(Parameters);

			return Function.FreeParameter(Parameters, State);
		}

		virtual void Bind(void* Self, lua_State* State, int AbsIndex) override
		{
			FMulticastScriptDelegate* Delegate = (FMulticastScriptDelegate*)Self;

			UTLuaCallback* Object = NewObject<UTLuaCallback>();
			Object->Bind(AbsIndex);
		//	Delegate->BindUFunction(Object, TEXT("Callback"));
			// OneMultiDelegate.AddDynamic(this, &UTLuaRootObject::OneReturn);

			FScriptDelegate InDelegate;
			InDelegate.BindUFunction(Object, TEXT("Callback"));
			Delegate->Add(InDelegate);
		}

	private:
		FunctionContext Function;
		FMulticastDelegateProperty* Property;
	};

	DelegateAccessor* CreateDelegateAccessor(FDelegateProperty* Property)
	{
		return new SingleDelegateAccessor(Property);
	}

	DelegateAccessor* CreateDelegateAccessor(FMulticastDelegateProperty* Property)
	{
		return new MulticastDelegateAccessor(Property);
	}
}
