
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
	}
}
