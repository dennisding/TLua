#pragma once

#include "CoreMinimal.h"

#include "TLuaImp.hpp"


namespace TLua {

	class PropertyBase
	{
	public:
		// virtual void PushValue(lua_State* State, const void* Container) = 0;
		virtual void SetLuaField(lua_State* State, int Index, const void* Container) = 0;
		virtual void GetLuaField(lua_State* State, int Index, void* Container) = 0;
	};

	using PropertyArray = TArray<PropertyBase*>;

	class UStructMgr
	{
		UStructMgr() {}
	public:

		template <typename StructType>
		static PropertyArray* Get()
		{
			return Get(TBaseStructure<StructType>::Get());
		}

		TLua_API static PropertyArray* Get(UStruct* Struct);

	private:
		static TMap<UStruct*, PropertyArray> StructInfos;
	};
}