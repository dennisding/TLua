
#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

#include "TLua.h"

namespace TLua
{
	template <typename Property>
	struct PropertyInfo {};

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
	};

	//template <>
	//struct PropertyInfo<FObjectProperty>
	//{
	//	using ValueType = TObjectPtr<UObject>;
	//	static ValueType GetValue(const UObject* Obj, FObjectProperty* Property)
	//	{
	//		ValueType Value;
	//		Property->GetValue_InContainer(Obj, &Value);
	//		return Value;
	//	}

	//	static void SetValue(UObject* Obj, FObjectProperty* Property, const ValueType& Value)
	//	{
	//		Property->SetValue_InContainer(Obj, Value);
	//	}
	//};

	template <typename DispatcherType>
	void DispatchProperty(FProperty* Property, DispatcherType &Dispatcher)
	{
		if (auto* IntProperty = CastField<FIntProperty>(Property))
		{
			Dispatcher.Visit(IntProperty);
			// Info->Parameters.Add(new ParameterType<FIntProperty>(IntProperty, ParamIndex));
		}
		else if (auto* FloatProperty = CastField<FFloatProperty>(Property))
		{
			Dispatcher.Visit(FloatProperty);
		}
		else if (auto* StrProperty = CastField<FStrProperty>(Property)) 
		{
			Dispatcher.Visit(StrProperty);
			// Info->Parameters.Add(new ParameterType<FStrProperty>(StrProperty, ParamIndex));
		}
		else {
			UE_LOG(Lua, Warning, TEXT("unhandled attribute:%s"), *Property->GetName());
		}
	}
}
