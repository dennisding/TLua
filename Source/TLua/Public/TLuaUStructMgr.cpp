
#include "TLuaUStructMgr.hpp"
#include "TLuaProperty.hpp"
#include "TLuaTypes.hpp"

namespace TLua
{
	StructTypeProcessor* UStructProcessorMgr::Get(UStruct* Struct)
	{
		static UStructProcessorMgr Mgr;

		return Mgr.GetProcessor(Struct);
	}

	StructTypeProcessor* UStructProcessorMgr::GetProcessor(UStruct* Struct)
	{
		auto Finded = Processors.find(Struct);
		if (Finded != Processors.end()) {
			return Finded->second;
		}

		StructTypeProcessor* Processor = new StructTypeProcessor(Struct);
		Processors[Struct] = Processor;

		return Processor;
	}

	StructTypeProcessor::~StructTypeProcessor()
	{
	}

	StructTypeProcessor::StructTypeProcessor(UStruct* InStruct) : Struct(InStruct)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It) {
			Properties.Add(CreatePropertyProcessor(*It));
		}
	}

	void StructTypeProcessor::FromLua(lua_State* State, int Index, void* Container)
	{
		for (PropertyProcessor* Processor : Properties) {
			LuaGetField(State, Index, Processor->GetAnsiName());
			Processor->FromLua(State, -1, Container);
			LuaPop(State);
		}
	}

	void StructTypeProcessor::ToLua(lua_State* State, const void* Container)
	{
		LuaNewTable(State);
		for (PropertyProcessor* Processor : Properties) {
			Processor->ToLua(State, Container);
			LuaSetField(State, -2, Processor->GetAnsiName());
		}
	}
}