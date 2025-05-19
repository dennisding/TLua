
#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

#include "TLua.h"
#include "TLuaTypes.hpp"

namespace TLua
{
	template <typename Property>
	struct PropertyInfo {};

	template <typename Type>
	inline void PushValue(lua_State* state, const Type& value);

	template <>
	struct PropertyInfo<FFloatProperty>
	{
		using ValueType = float;
		static float GetValue(const void* Obj, FFloatProperty* Property)
		{
			return Property->GetPropertyValue_InContainer(Obj);
		}

		static void SetValue(void* Obj, FFloatProperty* Property, float Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}

		static ValueType GetDefault(FFloatProperty* Property) 
		{
			return FCString::Atof(*Property->GetMetaData("CPP_DefaultValue"));
		}
	};

	template <>
	struct PropertyInfo<FDoubleProperty>
	{
		using ValueType = double;
		static ValueType GetValue(const void* Obj, FDoubleProperty* Property)
		{
			return Property->GetPropertyValue_InContainer(Obj);
		}

		static void SetValue(void* Obj, FDoubleProperty* Property, ValueType Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}

		static ValueType GetDefault(FDoubleProperty* Property)
		{
			return FCString::Atod(*Property->GetMetaData("CPP_DefaultValue"));
		}
	};

	template <>
	struct PropertyInfo<FIntProperty>
	{
		using ValueType = int32;
		static int32 GetValue(const void* Obj, FIntProperty* Property)
		{
			return Property->GetPropertyValue_InContainer(Obj);
		}

		static void SetValue(void* Obj, FIntProperty* Property, int32 value)
		{
			Property->SetValue_InContainer(Obj, value);
		}

		static ValueType GetDefault(FIntProperty* Property)
		{
			return FCString::Atoi(*Property->GetMetaData("CPP_DefaultValue"));
		}
	};

	template <>
	struct PropertyInfo<FBoolProperty>
	{
		using ValueType = bool;
		static ValueType GetValue(const void* Obj, FBoolProperty* Property)
		{
			return Property->GetPropertyValue_InContainer(Obj);
		}

		static void SetValue(void* Obj, FBoolProperty* Property, ValueType Value)
		{
			Property->SetPropertyValue_InContainer(Obj, (bool)Value);
		}

		static ValueType GetDefault(FBoolProperty* Property)
		{
			return Property->GetMetaData("CPP_DefaultValue").ToBool();
		}
	};

	template <>
	struct PropertyInfo<FStrProperty>
	{
		using ValueType = FString;
		static const FString& GetValue(const void* Obj, FStrProperty* Property)
		{
			return *(Property->GetPropertyValuePtr_InContainer(Obj));
		}

		static void SetValue(void* Obj, FStrProperty* Property, const FString& Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}

		static ValueType GetDefault(FStrProperty* Property)
		{
			return Property->GetMetaData("CPP_DefaultValue");
		}
	};

	template <>
	struct PropertyInfo<FNameProperty>
	{
		using ValueType = FName;
		static const FName& GetValue(const void* Obj, FNameProperty* Property)
		{
			return *(Property->GetPropertyValuePtr_InContainer(Obj));
		}

		static void SetValue(void* Obj, FNameProperty* Property, const FName& Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}

		static ValueType GetDefault(FNameProperty* Property)
		{
			return FName(*Property->GetMetaData("CPP_DefaultValue"));
		}
	};

	template <>
	struct PropertyInfo<FObjectProperty>
	{
		using ValueType = UObject*;
		static ValueType GetValue(const void* Obj, FObjectProperty* Property)
		{
			return Property->GetObjectPropertyValue_InContainer(Obj);
		}

		static void SetValue(void* Obj, FObjectProperty* Property, const ValueType& Value)
		{
			Property->SetObjectPropertyValue_InContainer(Obj, Value);
		}

		static ValueType GetDefault(FObjectProperty* Property)
		{
			return nullptr;
		}
	};

	template <typename DispatcherType>
	void DispatchProperty(FProperty* Property, DispatcherType &Dispatcher)
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
		else if (auto* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			UClass* PropertyClass = ObjectProperty->PropertyClass;
			if (PropertyClass->IsChildOf(UActorComponent::StaticClass()))
			{
				Dispatcher.Visit(ObjectProperty, (UActorComponent*)nullptr);
			}
			else if (PropertyClass->IsChildOf(AActor::StaticClass()))
			{
				Dispatcher.Visit(ObjectProperty, (AActor*)nullptr);
			}
			else 
			{
				UE_LOG(Lua, Error, TEXT("unhandled ObjectProperty:%s"), *Property->GetName());
			}
		}
		else {
			UE_LOG(Lua, Error, TEXT("unhandled attribute:%s"), *Property->GetName());
		}
	}
}
