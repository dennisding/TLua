#pragma once

#include "CoreMinimal.h"

#include "TLuaImp.hpp"
#include <map>


namespace TLua {

	//class PropertyBase
	//{
	//public:
	//	// virtual void PushValue(lua_State* State, const void* Container) = 0;
	//	virtual void SetLuaField(lua_State* State, int Index, const void* Container) = 0;
	//	virtual void GetLuaField(lua_State* State, int Index, void* Container) = 0;
	//};

	//using PropertyArray = TArray<PropertyBase*>;

	//class UStructMgr
	//{
	//	UStructMgr() {}
	//public:

	//	template <typename StructType>
	//	static PropertyArray* Get()
	//	{
	//		return Get(TBaseStructure<StructType>::Get());
	//	}

	//	TLua_API static PropertyArray* Get(UStruct* Struct);

	//private:
	//	static TMap<UStruct*, PropertyArray> StructInfos;
	//};

	class PropertyProcessor;
	PropertyProcessor* CreatePropertyProcessor(FProperty* Property);

	class StructTypeProcessor
	{
	public:
		virtual ~StructTypeProcessor();
		StructTypeProcessor(UStruct* InStruct);
		virtual void FromLua(lua_State* State, int Index, void* Container);
		virtual void ToLua(lua_State* State, const void* Container);
		//StructTypeProcessor(UStruct* InStruct) : Struct(InStruct)
		//{
		//	for (TFieldIterator<FProperty> It(Struct); It; ++It) {
		//		Properties.Add(CreatePropertyProcessor(*It));
		//	}
		//}


		//void FromLua(lua_State* State, int Index, void* Container)
		//{
		//	for (PropertyProcessor* Processor : Properties) {
		//		LuaGetField(State, Index, Processor->GetAnsiName());
		//		Processor->FromLua(State, -1, Container);
		//		LuaPop(State);
		//	}

		//}

		//void ToLua(lua_State* State, const void* Container)
		//{
		//	LuaNewTable(State);
		//	for (PropertyProcessor* Processor : Properties) {
		//		Processor->ToLua(State, Container);
		//		LuaSetField(State, -2, Processor->GetAnsiName());
		//	}
		//}

	private:
		UStruct* Struct;
		TArray<PropertyProcessor*> Properties;
	};

	class UStructProcessorMgr
	{
		UStructProcessorMgr() {}

	public:
		template <typename StructType>
		static StructTypeProcessor* Get()
		{
			return Get(TBaseStructure<StructType>::Get());
		}

		TLua_API static StructTypeProcessor* Get(UStruct* Struct);

		StructTypeProcessor* GetProcessor(UStruct* Struct);

	protected:
		std::map<UStruct*, StructTypeProcessor*> Processors;
	};
}