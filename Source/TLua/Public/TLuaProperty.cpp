
#include "TLuaProperty.hpp"

#include "TLuaCppLua.hpp"

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

	class DelegateProcessor : public DelegateProcessorBase
	{
	public:
		DelegateProcessor(FDelegateProperty* InProperty) 
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

	private:
		FunctionContext Function;
		FDelegateProperty* Property;
	};

	DelegateProcessorBase* CreateDelegateProcessor(FDelegateProperty* Property)
	{
		return new DelegateProcessor(Property);
	}

	DelegateProcessorBase* CreateMulticastDelegateProcessor(FMulticastDelegateProperty* Property)
	{
		return nullptr;
	}
}
