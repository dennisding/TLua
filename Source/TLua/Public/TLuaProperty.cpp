
#include "TLuaProperty.hpp"

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
}
