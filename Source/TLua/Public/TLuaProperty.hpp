
#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

#include "TLua.h"
#include "TLuaPropertyInfo.hpp"
#include "TLuaTypeInfo.hpp"
#include <string>

namespace TLua
{
	class PropertyProcessor
	{
	public:
		PropertyProcessor(FProperty* InProperty) : Property(InProperty)
		{
			FString OriginName = Property->GetName();
			AnsiName = std::string(TCHAR_TO_ANSI(*OriginName));
		}

		virtual ~PropertyProcessor() {}

		virtual void FromLua(lua_State* State, int Index, void* OutValue) = 0;
		virtual void ToLua(lua_State* State, const void* Value) = 0;

	public:
		inline const char* GetAnsiName()
		{
			return AnsiName.c_str();
		}

		inline void DestroyValue_InContainer(void* Container)
		{
			Property->DestroyValue_InContainer(Container);
		}

	private:
		FProperty* Property;
		std::string AnsiName;
	};

	using ProcessorArray = TArray<PropertyProcessor*>;

	PropertyProcessor* CreatePropertyProcessor(FProperty* Property);

	template <typename PropertyType, typename ValueType = PropertyInfo<PropertyType>::ValueType>
	class Processor : public PropertyProcessor
	{
	public:
		virtual ~Processor() {}
		Processor(PropertyType* InProperty)
			: PropertyProcessor(InProperty), Property(InProperty)
		{
		}

		virtual void FromLua(lua_State* State, int Index, void* Container) override
		{
			ValueType* ValuePtr = Property->ContainerPtrToValuePtr<ValueType>(Container);
			*ValuePtr = TypeInfo<ValueType>::FromLua(State, Index);
		}

		virtual void ToLua(lua_State* State, const void* Container) override
		{
			const ValueType* ValuePtr = Property->ContainerPtrToValuePtr<ValueType>(Container);
			TypeInfo<ValueType>::ToLua(State, *ValuePtr);
		}

	private:
		PropertyType* Property;
	};

	template <>
	class Processor<FStructProperty, PropertyInfo<FStructProperty>::ValueType> 
		: public PropertyProcessor
	{
	public:
		virtual ~Processor() {}

		Processor(FStructProperty* InProperty) 
			: PropertyProcessor(InProperty), Property(InProperty), StructProcessor(nullptr)
		{
			UStruct* Struct = InProperty->Struct;
			StructProcessor = UStructProcessorMgr::Get(Struct);
		}

		virtual void FromLua(lua_State* State, int Index, void* Container) override
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);
			StructProcessor->FromLua(State, Index, ValuePtr);
		}

		virtual void ToLua(lua_State* State, const void* Container) override
		{
			const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);
			StructProcessor->ToLua(State, ValuePtr);
		}

	private:
		FStructProperty* Property;
		StructTypeProcessor* StructProcessor;
	};

	template <>
	class Processor<FEnumProperty, PropertyInfo<FEnumProperty>::ValueType> 
		: public PropertyProcessor
	{
	public:
		virtual ~Processor() {}

		Processor(FEnumProperty* InProperty) 
			: PropertyProcessor(InProperty), Property(InProperty)
		{
			UnderlyingProcessor = CreatePropertyProcessor(Property->GetUnderlyingProperty());
		}

		virtual void FromLua(lua_State* State, int Index, void* Container) override
		{
			UnderlyingProcessor->FromLua(State, Index, Container);
		}

		virtual void ToLua(lua_State* State, const void* Container) override
		{
			UnderlyingProcessor->ToLua(State, Container);
		}

	private:
		PropertyProcessor* UnderlyingProcessor;
		FEnumProperty* Property;
	};

	template <typename DispatcherType>
	void DispatchProperty(FProperty* Property, DispatcherType& Dispatcher)
	{
		if (auto* IntProperty = CastField<FIntProperty>(Property))
		{
			Dispatcher.Visit(IntProperty);
		}
		else if (auto* FloatProperty = CastField<FFloatProperty>(Property))
		{
			Dispatcher.Visit(FloatProperty);
		}
		else if (auto* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			Dispatcher.Visit(DoubleProperty);
		}
		else if (auto* StrProperty = CastField<FStrProperty>(Property))
		{
			Dispatcher.Visit(StrProperty);
		}
		else if (auto* BoolProperty = CastField<FBoolProperty>(Property))
		{
			Dispatcher.Visit(BoolProperty);
		}
		else if (auto* NameProperty = CastField<FNameProperty>(Property))
		{
			Dispatcher.Visit(NameProperty);
		}
		else if (auto* EnumProperty = CastField<FEnumProperty>(Property))
		{
			Dispatcher.Visit(EnumProperty);
		}
		else if (auto* ByteProperty = CastField<FByteProperty>(Property))
		{
			Dispatcher.Visit(ByteProperty);
		}
		else if (auto* StructProperty = CastField<FStructProperty>(Property))
		{
			Dispatcher.Visit(StructProperty);
		}
		else if (auto* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			Dispatcher.Visit(ObjectProperty);
		}
		else {
			UE_LOG(Lua, Error, TEXT("unhandled attribute:%s"), *Property->GetName());
		}
	}

	class ProcessorVisitor
	{
	public:
		ProcessorVisitor();

		template <typename PropertyType>
		void Visit(PropertyType* Property)
		{
			using ValueType = PropertyInfo<PropertyType>::ValueType;
			Result = new Processor<PropertyType, ValueType>(Property);
		}

		void Visit(FObjectProperty* Property);

	public:
		PropertyProcessor* Result;
	};

	inline PropertyProcessor* CreatePropertyProcessor(FProperty* Property)
	{
		ProcessorVisitor Visitor;
		DispatchProperty(Property, Visitor);
		return Visitor.Result;
	}
}
