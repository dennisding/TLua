
#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

namespace TLua
{
	template <typename Property>
	struct PropertyInfo {};

	template <>
	struct PropertyInfo<FFloatProperty>
	{
		using ValueType = float;
		static float GetValue(UObject* Obj, FFloatProperty* Property)
		{
			return Property->GetPropertyValue_InContainer(Obj);
		}

		static void SetValue(UObject* Obj, FFloatProperty* Property, float Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}
	};

	template <>
	struct PropertyInfo<FIntProperty>
	{
		using ValueType = int32;
		static int32 GetValue(void* Obj, FIntProperty* Property)
		{
			return Property->GetPropertyValue_InContainer(Obj);
		}

		static void SetValue(void* Obj, FIntProperty* Property, int32 value)
		{
			Property->SetValue_InContainer(Obj, value);
		}
	};

	template <>
	struct PropertyInfo<FStrProperty>
	{
		using ValueType = FString;
		static FString& GetValue(UObject* Obj, FStrProperty* Property)
		{
			return *(Property->GetPropertyValuePtr_InContainer(Obj));
		}

		static void SetValue(UObject* Obj, FStrProperty* Property, const FString& Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}
	};

	template <>
	struct PropertyInfo<FObjectProperty>
	{
		using ValueType = TObjectPtr<UObject>;
		static ValueType GetValue(UObject* Obj, FObjectProperty* Property)
		{
			ValueType Value;
			Property->GetValue_InContainer(Obj, &Value);
			return Value;
		}

		static void SetValue(UObject* Obj, FObjectProperty* Property, const ValueType& Value)
		{
			Property->SetValue_InContainer(Obj, Value);
		}

	};
}
