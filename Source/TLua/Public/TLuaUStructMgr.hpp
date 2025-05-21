#pragma once

#include "CoreMinimal.h"

#include "TLuaImp.hpp"
#include <map>


namespace TLua {

	class PropertyProcessor;
	PropertyProcessor* CreatePropertyProcessor(FProperty* Property);

	class StructTypeProcessor
	{
	public:
		virtual ~StructTypeProcessor();
		StructTypeProcessor(UStruct* InStruct);
		virtual void FromLua(lua_State* State, int Index, void* Container);
		virtual void ToLua(lua_State* State, const void* Container);

	private:
		UStruct* Struct;
		TArray<PropertyProcessor*> Properties;
	};

	class UStructProcessorMgr
	{
		inline UStructProcessorMgr() {}

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