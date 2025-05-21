
#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

#include "TLua.h"
#include "TLuaPropertyInfo.hpp"
#include "TLuaTypeInfo.hpp"
// #include "TLuaTypes.hpp"
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
//		using ValueType = PropertyInfo<PropertyType>::ValueType;
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

	//template <typename ValueType>
	//class Processor<FObjectProperty, ValueType> : public PropertyProcessor
	//{
	//public:
	//	virtual ~Processor() {}
	//	Processor(FObjectProperty* InProperty)
	//		: PropertyProcessor(InProperty), Property(InProperty)
	//	{
	//		//FString OriginName = Property->GetName();
	//		//Name = std::string(TCHAR_TO_ANSI(*OriginName));
	//	}

	//	virtual void FromLua(lua_State* State, int Index, void* Container) override
	//	{
	//		ValueType* ValuePtr = Property->ContainerPtrToValuePtr<ValueType>(Container);
	//		*ValuePtr = TypeInfo<ValueType>::FromLua(State, Index);
	//	}

	//	virtual void ToLua(lua_State* State, const void* Container) override
	//	{
	//		const ValueType* ValuePtr = Property->ContainerPtrToValuePtr<ValueType>(Container);
	//		TypeInfo<ValueType>::ToLua(State, *ValuePtr);
	//	}

	//private:
	//	FObjectProperty* Property;
	//};

	template <>
	class Processor<FStructProperty, PropertyInfo<FStructProperty>::ValueType> 
		: public PropertyProcessor
	{
	public:
		virtual ~Processor() {}
		//Processor(FStructProperty* InProperty)
		//	: PropertyProcessor(InProperty), Property(InProperty)
		//{
		//	for (TFieldIterator<FProperty> It(Property->Struct); It; ++It) {
		//		ChildProcessors.Add(CreatePropertyProcessor(*It));
		//	}
		//}

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

			//void* ValuePtr = Property->ContainerPtrToValuePtr<void*>(Container);
			//for (PropertyProcessor* Processor : ChildProcessors) {
			//	LuaGetField(State, Index, Processor->GetAnsiName());
			//	Processor->FromLua(State, -1, ValuePtr);
			//	LuaPop(State);
			//}
		}

		virtual void ToLua(lua_State* State, const void* Container) override
		{
			const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);
			StructProcessor->ToLua(State, ValuePtr);
			//const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);

			//LuaNewTable(State);
			//for (PropertyProcessor* Processor : ChildProcessors) {
			//	Processor->ToLua(State, ValuePtr);
			//	LuaSetField(State, -2, Processor->GetAnsiName());
			//}
		}

	private:
		FStructProperty* Property;
		StructTypeProcessor* StructProcessor;
//		ProcessorArray ChildProcessors;
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
			//UClass* PropertyClass = ObjectProperty->PropertyClass;
			//if (PropertyClass->IsChildOf(UActorComponent::StaticClass()))
			//{
			//	Dispatcher.Visit(ObjectProperty, (UActorComponent*)nullptr);
			//}
			//else if (PropertyClass->IsChildOf(AActor::StaticClass()))
			//{
			//	Dispatcher.Visit(ObjectProperty, (AActor*)nullptr);
			//}
			//else 
			//{
			//	UE_LOG(Lua, Error, TEXT("unhandled ObjectProperty:%s"), *Property->GetName());
			//}
		}
		else {
			UE_LOG(Lua, Error, TEXT("unhandled attribute:%s"), *Property->GetName());
		}
	}

	class ProcessorVisitor
	{
	public:
		ProcessorVisitor() : Result(nullptr)
		{

		}

		template <typename PropertyType>
		void Visit(PropertyType* Property)
		{
			using ValueType = PropertyInfo<PropertyType>::ValueType;
			Result = new Processor<PropertyType, ValueType>(Property);
		}

		void Visit(FObjectProperty* Property)
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

		PropertyProcessor* Result;
	};

	inline PropertyProcessor* CreatePropertyProcessor(FProperty* Property)
	{
		ProcessorVisitor Visitor;
		DispatchProperty(Property, Visitor);
		return Visitor.Result;
	}

	//template <>
	//struct PropertyInfo<FFloatProperty>
	//{
	//	using ValueType = float;
	//	static float GetValue(const void* Obj, FFloatProperty* Property)
	//	{
	//		return Property->GetPropertyValue_InContainer(Obj);
	//	}

	//	static void SetValue(void* Obj, FFloatProperty* Property, float Value)
	//	{
	//		Property->SetValue_InContainer(Obj, Value);
	//	}

	//	static ValueType GetDefault(FFloatProperty* Property) 
	//	{
	//		return FCString::Atof(*Property->GetMetaData("CPP_DefaultValue"));
	//	}
	//};

//	template <>
//	struct PropertyInfo<FDoubleProperty>
//	{
//		using ValueType = double;
//		static ValueType GetValue(const void* Obj, FDoubleProperty* Property)
//		{
//			return Property->GetPropertyValue_InContainer(Obj);
//		}
//
//		static void SetValue(void* Obj, FDoubleProperty* Property, ValueType Value)
//		{
//			Property->SetValue_InContainer(Obj, Value);
//		}
//
//		static ValueType GetDefault(FDoubleProperty* Property)
//		{
//			return FCString::Atod(*Property->GetMetaData("CPP_DefaultValue"));
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FIntProperty>
//	{
//		using ValueType = int32;
//		static int32 GetValue(const void* Obj, FIntProperty* Property)
//		{
//			return Property->GetPropertyValue_InContainer(Obj);
//		}
//
//		static void SetValue(void* Obj, FIntProperty* Property, int32 value)
//		{
//			Property->SetValue_InContainer(Obj, value);
//		}
//
//		static ValueType GetDefault(FIntProperty* Property)
//		{
//			return FCString::Atoi(*Property->GetMetaData("CPP_DefaultValue"));
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FByteProperty>
//	{
//		using ValueType = int32;
//		static ValueType GetValue(const void* Obj, FByteProperty* Property)
//		{
//			return Property->GetPropertyValue_InContainer(Obj);
//		}
//
//		static void SetValue(void* Obj, FByteProperty* Property, ValueType value)
//		{
//			Property->SetValue_InContainer(Obj, value);
//		}
//
//		static ValueType GetDefault(FByteProperty* Property)
//		{
//			return FCString::Atoi(*Property->GetMetaData("CPP_DefaultValue"));
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FEnumProperty>
//	{
//		using ValueType = int32;
//		static ValueType GetValue(const void* Obj, FEnumProperty* Property)
//		{
//			//FNumericProperty* RealProperty = Property->GetUnderlyingProperty();
//			//return RealProperty->GetPropertyValue_InContainer(Obj);
//			const void* ValuePtr = Property->ContainerPtrToValuePtr<void*>(Obj);
//			FNumericProperty* RealProperty = Property->GetUnderlyingProperty();
//			int64 CurrentValue = RealProperty->GetSignedIntPropertyValue(ValuePtr);
//
//			return (ValueType)CurrentValue;
//		}
//
//		static void SetValue(void* Obj, FEnumProperty* Property, ValueType value)
//		{
//			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Obj);
//			FNumericProperty* RealProperty = Property->GetUnderlyingProperty();
//			RealProperty->SetIntPropertyValue(ValuePtr, (int64)value);
//		}
//
//		static ValueType GetDefault(FEnumProperty* Property)
//		{
//			FNumericProperty* RealProperty = Property->GetUnderlyingProperty();
//			UEnum* Enum = Property->GetEnum();
//			auto Result = Enum->GetValueByName(*Property->GetMetaData("CPP_DefaultValue"));
//			//return FCString::Atoi(*RealProperty->GetMetaData("CPP_DefaultValue"));
//			return (ValueType)Result;
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FBoolProperty>
//	{
//		using ValueType = bool;
//		static ValueType GetValue(const void* Obj, FBoolProperty* Property)
//		{
//			return Property->GetPropertyValue_InContainer(Obj);
//		}
//
//		static void SetValue(void* Obj, FBoolProperty* Property, ValueType Value)
//		{
//			Property->SetPropertyValue_InContainer(Obj, (bool)Value);
//		}
//
//		static ValueType GetDefault(FBoolProperty* Property)
//		{
//			return Property->GetMetaData("CPP_DefaultValue").ToBool();
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FStrProperty>
//	{
//		using ValueType = FString;
//		static const FString& GetValue(const void* Obj, FStrProperty* Property)
//		{
//			return *(Property->GetPropertyValuePtr_InContainer(Obj));
//		}
//
//		static void SetValue(void* Obj, FStrProperty* Property, const FString& Value)
//		{
//			Property->SetValue_InContainer(Obj, Value);
//		}
//
//		static ValueType GetDefault(FStrProperty* Property)
//		{
//			return Property->GetMetaData("CPP_DefaultValue");
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FNameProperty>
//	{
//		using ValueType = FName;
//		static const FName& GetValue(const void* Obj, FNameProperty* Property)
//		{
//			return *(Property->GetPropertyValuePtr_InContainer(Obj));
//		}
//
//		static void SetValue(void* Obj, FNameProperty* Property, const FName& Value)
//		{
//			Property->SetValue_InContainer(Obj, Value);
//		}
//
//		static ValueType GetDefault(FNameProperty* Property)
//		{
//			return FName(*Property->GetMetaData("CPP_DefaultValue"));
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FObjectProperty>
//	{
//		using ValueType = UObject*;
//		static ValueType GetValue(const void* Obj, FObjectProperty* Property)
//		{
//			return Property->GetObjectPropertyValue_InContainer(Obj);
//		}
//
//		static void SetValue(void* Obj, FObjectProperty* Property, const ValueType& Value)
//		{
//			Property->SetObjectPropertyValue_InContainer(Obj, Value);
//		}
//
//		static ValueType GetDefault(FObjectProperty* Property)
//		{
//			return nullptr;
//		}
//	};
//
//	template <>
//	struct PropertyInfo<FStructProperty>
//	{
//		using ValueType = void*;
//		static TSharedPtr<void> GetValue(const void* Container, FStructProperty* Property)
//		{
//
//			TSharedPtr<void> Ptr = CreateInstance(Property);
//			if (!Ptr) {
//				return Ptr;
//			}
//
//			const void* SourcePtr = Property->ContainerPtrToValuePtr<void*>(Container);
//			Property->CopySingleValue(Ptr.Get(), SourcePtr);
//
//			return Ptr;
//			//if (!Property || !Property->Struct)
//			//{
//			//	return nullptr;
//			//}
//
//			//auto Struct = Property->Struct;
//
//			//// allocate the structure
//			//void* StructInstance = FMemory::Malloc(Struct->GetStructureSize());
//			//Struct->InitializeStruct(StructInstance);
//
//			//const void* SourcePtr = Property->ContainerPtrToValuePtr<void*>(Container);
//			//Property->CopySingleValue(StructInstance, SourcePtr);
//
//			//// return shared ptr and auto delete.
//			//return TSharedPtr<void>(StructInstance, [Struct](void* ObjToDelete)
//			//	{
//			//		Struct->DestroyStruct(ObjToDelete);
//			//		FMemory::Free(ObjToDelete);
//			//	});
////			return nullptr;
//		}
//
//		static void SetValue(void* Object, FStructProperty* Property, const ValueType& Value)
//		{
//			//UScriptStruct* Struct = Property->Struct;
//			//PropertyArray* Fields = UStructMgr::Get(Struct);
//
//			//void* StructPtr = Property->ContainerPtrToValuePtr<void*>(Obj);
//
//			//for (auto Field : *Fields) {
//			//	void* FieldPtr = Field->Property->ContainerPtrToValuePtr(Struct);
//			//}
//			void* DestPtr = Property->ContainerPtrToValuePtr<void*>(Object);
//			Property->CopySingleValue(DestPtr, Value);
//		}
//
//		static TSharedPtr<void> GetDefault(FStructProperty* Property)
//		{
//			TSharedPtr<void> Ptr = CreateInstance(Property);
//			
//			return Ptr;
//		}
//
//	private:
//		static TSharedPtr<void> CreateInstance(FStructProperty* Property)
//		{
//			if (!Property || !Property->Struct)
//			{
//				return nullptr;
//			}
//
//			auto Struct = Property->Struct;
//
//			// allocate the structure
//			void* StructInstance = FMemory::Malloc(Struct->GetStructureSize());
//			Struct->InitializeStruct(StructInstance);
//
//			// return shared ptr and auto delete.
//			return TSharedPtr<void>(StructInstance, [Struct](void* ObjToDelete)
//				{
//					Struct->DestroyStruct(ObjToDelete);
//					FMemory::Free(ObjToDelete);
//				});
//		}
//	};
}
