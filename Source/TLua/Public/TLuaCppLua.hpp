
#pragma once

#include "Lua/lua.hpp"

#include "TLuaCall.hpp"
#include "TLuaProperty.hpp"

namespace TLua
{
	class FunctionContext
	{
	public:
		FunctionContext(UFunction* InFunction);

		// _cpp_object_call(Object, Context, args...)
		int Call(lua_State* State, UObject* Object);

		void FillParameters(void* Parameters, lua_State* State, int ArgStartIndex);
		int FreeParameter(void* Parameters, lua_State* State);

	private:
		void ProcessParameterProperty();
		void ProcessReturnProperty();

	private:
		UFunction* Function;
		PropertyProcessor* Return;
		ProcessorArray ParameterProcessors;
	};

	void RegisterCppLua();
}
