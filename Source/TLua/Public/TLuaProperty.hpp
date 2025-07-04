
#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

#include "TLua.h"
#include "TLuaPropertyInfo.hpp"
#include "TLuaTypeInfo.hpp"
#include "TLuaTypes.hpp"
#include <string>

namespace TLua
{
	class PropertyProcessor
	{
	public:
		inline PropertyProcessor(FProperty* InProperty) : Property(InProperty)
		{
			FString OriginName = Property->GetName();
			AnsiName = std::string(TCHAR_TO_ANSI(*OriginName));
		}

		virtual ~PropertyProcessor() {}

		virtual void FromLua(lua_State* State, int Index, void* OutValue) = 0;
		virtual void ToLua(lua_State* State, const void* Value) = 0;
		inline virtual void ReturnToLua(lua_State* State, const void* Value)
		{
			ToLua(State, Value);
		}

		inline virtual void DestroyValue(void* Container, lua_State* State, int Index)
		{
			DestroyValue_InContainer(Container);
		}

	public:
		inline const char* GetAnsiName()
		{
			return AnsiName.c_str();
		}

		inline void DestroyValue_InContainer(void* Container)
		{
			Property->DestroyValue_InContainer(Container);
		}

	public:
		FProperty* Property;
		std::string AnsiName;
	};

	using ProcessorArray = TArray<PropertyProcessor*>;

	PropertyProcessor* CreatePropertyProcessor(FProperty* Property);

	class DelegateAccessor
	{
	public:
		virtual ~DelegateAccessor() {}
		virtual int Execute(void* Self, lua_State* State, int ArgStartIndex) = 0;
		virtual void Bind(void* Self, lua_State* State, int Index) = 0;
	private:
	};

	DelegateAccessor* CreateDelegateAccessor(FDelegateProperty* Property);
	DelegateAccessor* CreateDelegateAccessor(FMulticastDelegateProperty* Property);

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
			Property->SetPropertyValue_InContainer(Container, 
						TypeInfo<ValueType>::FromLua(State, Index));
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
			: PropertyProcessor(InProperty), Property(InProperty)
		{
		}

		virtual void FromLua(lua_State* State, int Index, void* Container) override
		{
			if (!LuaIsTable(State, Index)) {
				return;
			}

			LuaGetField(State, Index, "_co");
			void* Source = (void*)LuaGetUserData(State, -1);
			LuaPop(State, 1);

			Property->SetValue_InContainer(Container, Source);
		}

		virtual void ToLua(lua_State* State, const void* Container) override
		{
			const void* Value = Property->ContainerPtrToValuePtr<void>(Container);
			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_struct");
			LuaPushUserData(State, (void*)Value);
			LuaPushUserData(State, (void*)Property->Struct);
			LuaPCall(State, 3, 1);
		}

		virtual void ReturnToLua(lua_State* State, const void* Container) override
		{
			const void* Source = Property->ContainerPtrToValuePtr<void>(Container);
			void* Value = LuaNewUserData(State, Property->Struct->GetStructureSize(), 0);
			Property->CopyCompleteValue(Value, Source); 

			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_struct");
			LuaPushUserData(State, (void*)Value);
			LuaPushUserData(State, (void*)Property->Struct);
			LuaPushUserData(State, (void*)Property);
			LuaPCall(State, 4, 1);
		}

		virtual void DestroyValue(void* Container, lua_State* State, int Index) override
		{
			if (!LuaIsTable(State, Index)) {
				Property->DestroyValue_InContainer(Container);
				return;
			}

			if (Property->HasAnyPropertyFlags(CPF_ReferenceParm)) {
				// copy back the reference
				LuaGetField(State, Index, "_co");
				void* LuaValue = (void*)LuaGetUserData(State, -1);
				LuaPop(State, 1);

				void* CppValue = Property->ContainerPtrToValuePtr<void*>(Container);
				Property->CopyCompleteValue(LuaValue, CppValue);
			}
			
			Property->DestroyValue_InContainer(Container);
		}

	private:
		FStructProperty* Property;
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

	template <>
	class Processor<FArrayProperty, void> : public PropertyProcessor
	{
	public:
		virtual ~Processor() {}

		Processor(FArrayProperty* InProperty) 
			: PropertyProcessor(InProperty), Property(InProperty)
		{
			InnerProcessor = CreatePropertyProcessor(Property->Inner);
		}

		virtual void FromLua(lua_State* State, int Index, void* Container) override
		{
			if (!LuaIsTable(State, Index)) {
				return;
			}

			void* ArrayPtr = Property->ContainerPtrToValuePtr<void>(Container);
			FScriptArrayHelper Array(Property, ArrayPtr);
			if (Array.Num() != 0) {
				Array.RemoveValues(0, Array.Num());
			}

			int Size = LuaGetTableSize(State, Index);
			for (int ArrayIndex = 0; ArrayIndex < Size; ++ArrayIndex) {
				int ItemIndex = Array.AddValue();
				void* ItemPtr = Array.GetRawPtr(ItemIndex);

				LuaGetI(State, Index, ArrayIndex + 1);
				InnerProcessor->FromLua(State, -1, ItemPtr);
				LuaPop(State);
			}
		}

		virtual void ToLua(lua_State* State, const void* Container) override
		{
			const void* ArrayPtr = Property->ContainerPtrToValuePtr<void>(Container);
			FScriptArrayHelper Array(Property, ArrayPtr);

			LuaNewTable(State);
			for (int Index = 0; Index < Array.Num(); ++Index) {
				void* ItemPtr = Array.GetRawPtr(Index);
				LuaPushInteger(State, Index + 1);
				InnerProcessor->ToLua(State, ItemPtr);
				LuaSetTable(State, -3);
			}
		}

	private:
		FArrayProperty* Property;
		PropertyProcessor* InnerProcessor;
	};

	template <typename DelegateType>
	class TDelegateProcessor : public PropertyProcessor
	{
	public:
		virtual ~TDelegateProcessor() {}

		virtual void FromLua(lua_State* State, int Index, void* Container) override
		{

		}

		TDelegateProcessor(DelegateType* InProperty)
			: PropertyProcessor(InProperty), Property(InProperty)
		{
			InnerAccessor = CreateDelegateAccessor(Property);
		}

		virtual void ToLua(lua_State* State, const void* Container)
		{
			auto* DelegatePtr = Property->ContainerPtrToValuePtr<void>(Container);
			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_delegate");
			LuaPushUserData(State, (void*)DelegatePtr);
			LuaPushUserData(State, (void*)InnerAccessor);
			LuaPCall(State, 3, 1);
		}

	private:
		DelegateAccessor* InnerAccessor;
		DelegateType* Property;
	};

	template <typename DispatcherType>
	void DispatchProperty(FProperty* Property, DispatcherType& Dispatcher)
	{
		if (auto* Int8Property = CastField<FInt8Property>(Property)) {
			Dispatcher.Visit(Int8Property);
		}
		else if (auto* Int16Property = CastField<FInt16Property>(Property)) {
			Dispatcher.Visit(Int16Property);
		}
		else if (auto* IntProperty = CastField<FIntProperty>(Property))
		{
			Dispatcher.Visit(IntProperty);
		}
		else if (auto* Int64Property = CastField<FInt64Property>(Property)) {
			Dispatcher.Visit(Int64Property);
		}
		else if (auto* ByteProperty = CastField<FByteProperty>(Property))
		{
			Dispatcher.Visit(ByteProperty);
		}
		else if (auto* UInt16Property = CastField<FUInt16Property>(Property)) {
			Dispatcher.Visit(UInt16Property);
		}
		else if (auto* UInt32Property = CastField<FUInt32Property>(Property)) {
			Dispatcher.Visit(UInt32Property);
		}
		else if (auto* UInt64Property = CastField<FUInt64Property>(Property)) {
			Dispatcher.Visit(UInt64Property);
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
		else if (auto* TextProperty = CastField<FTextProperty>(Property)) {
			Dispatcher.Visit(TextProperty);
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
		else if (auto* StructProperty = CastField<FStructProperty>(Property))
		{
			Dispatcher.Visit(StructProperty);
		}
		else if (auto* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			Dispatcher.Visit(ObjectProperty);
		}
		else if (auto* WeakObjectProperty = CastField<FWeakObjectProperty>(Property)) {
			Dispatcher.Visit(WeakObjectProperty);
		}
		else if (auto* SoftObjectProperty = CastField<FSoftObjectProperty>(Property)) {
			Dispatcher.Visit(SoftObjectProperty);
		}
		else if (auto* ArrayProperty = CastField<FArrayProperty>(Property)) {
			Dispatcher.Visit(ArrayProperty);
		}
		else if (auto* DelegateProperty = CastField<FDelegateProperty>(Property)) {
			Dispatcher.Visit(DelegateProperty);
		}
		else if (auto* MulticastDelegate = CastField<FMulticastDelegateProperty>(Property)) {
			Dispatcher.Visit(MulticastDelegate);
		}
		else {
			UE_LOG(Lua, Error, TEXT("unhandled attribute:%s<%s>"), 
				*Property->GetName(), *Property->GetClass()->GetName());
			//Dispatcher.Visit(Property);
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
		void Visit(FArrayProperty* Property);
		void Visit(FDelegateProperty* Property);
		void Visit(FMulticastDelegateProperty* Property);

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
