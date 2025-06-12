
#pragma once

#include "Lua/lua.hpp"

#include "TLuaCall.hpp"
#include "TLuaProperty.hpp"

namespace TLua
{
	class FunctionContext
	{
	public:
		FunctionContext(UFunction* InFunction); //: Function(InFunction), Return(nullptr)
		//{
		//	ProcessParameterProperty();
		//	ProcessReturnProperty();
		//}

		// _cpp_object_call(Object, Context, args...)
		int Call(lua_State* State, UObject* Object);
		//{
		//	// setup parameter
		//	void* ParameterMemory = (void*)FMemory_Alloca(Function->ParmsSize);
		//	FMemory::Memzero(ParameterMemory, Function->ParmsSize);

		//	int LuaTop = LuaGetTop(State);
		//	for (int Index = 0; Index < ParameterProcessors.Num(); ++Index) {
		//		// _cpp_object_call_fun(self, fun_context, args...)
		//		PropertyProcessor* Processor = ParameterProcessors[Index];

		//		int LuaIndex = Index + 3;
		//		if (LuaIndex <= LuaTop) {
		//			Processor->FromLua(State, LuaIndex, ParameterMemory);
		//		}
		//		else {
		//			Processor->Property->InitializeValue_InContainer(ParameterMemory);
		//		}
		//	}

		//	// call the function
		//	Object->ProcessEvent(Function, ParameterMemory);

		//	// free parameter
		//	for (PropertyProcessor* Processor : ParameterProcessors) {
		//		Processor->DestroyValue_InContainer(ParameterMemory);
		//	}

		//	// process return
		//	if (Return) {
		//		Return->ToLua(State, ParameterMemory);
		//		Return->DestroyValue_InContainer(ParameterMemory);
		//		return 1;
		//	}

		//	return 0;
		//}

		void FillParameters(void* Parameters, lua_State* State, int ArgStartIndex);
		int FreeParameter(void* Parameters, lua_State* State);

	private:
		void ProcessParameterProperty();
		//{
		//	for (TFieldIterator<FProperty> It(Function); It; ++It) {
		//		FProperty* Property = *It;

		//		if (Property->HasAnyPropertyFlags(CPF_ReturnParm)) {
		//			continue;
		//		}
		//		// add the processor
		//		ParameterProcessors.Add(CreatePropertyProcessor(Property));
		//	}
		//}

		void ProcessReturnProperty();
		//{
		//	FProperty* Property = Function->GetReturnProperty();
		//	if (!Property) {
		//		return;
		//	}

		//	Return = CreatePropertyProcessor(Property);
		//}

	private:
		UFunction* Function;
		PropertyProcessor* Return;
		ProcessorArray ParameterProcessors;
	};

	void RegisterCppLua();
}
