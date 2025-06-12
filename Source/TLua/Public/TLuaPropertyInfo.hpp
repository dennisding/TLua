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
	};

	template <>
	struct PropertyInfo<FDoubleProperty>
	{
		using ValueType = double;
	};

	template <>
	struct PropertyInfo<FBoolProperty>
	{
		using ValueType = bool;
	};

	template <>
	struct PropertyInfo<FByteProperty>
	{
		using ValueType = uint8;
	};

	template <>
	struct PropertyInfo<FInt8Property>
	{
		using ValueType = int8;
	};

	template <>
	struct PropertyInfo<FInt16Property>
	{
		using ValueType = int16;
	};

	template <>
	struct PropertyInfo<FIntProperty>
	{
		using ValueType = int32;
	};

	template <>
	struct PropertyInfo<FInt64Property>
	{
		using ValueType = int64;
	};

	template <>
	struct PropertyInfo<FUInt16Property>
	{
		using ValueType = uint16;
	};

	template <>
	struct PropertyInfo<FUInt32Property>
	{
		using ValueType = uint32;
	};

	template <>
	struct PropertyInfo<FUInt64Property>
	{
		using ValueType = uint64;
	};

	template <>
	struct PropertyInfo<FNameProperty>
	{
		using ValueType = FName;
	};

	template <>
	struct PropertyInfo<FStrProperty>
	{
		using ValueType = FString;
	};

	template <>
	struct PropertyInfo<FTextProperty>
	{
		using ValueType = FText;
	};

	template <>
	struct PropertyInfo<FObjectProperty>
	{
//		using ValueType = UObject*;
		using ValueType = FObjectPtr;
	};

	template <>
	struct PropertyInfo<FSoftObjectProperty>
	{
		using ValueType = FSoftObjectPtr;
	};

	template <>
	struct PropertyInfo<FWeakObjectProperty>
	{
		using ValueType = FWeakObjectPtr;
	};

	template <>
	struct PropertyInfo<FStructProperty>
	{
		/// using ValueType = ? ? ? ;
		using ValueType = void*;
	};

	template <>
	struct PropertyInfo<FEnumProperty>
	{
		// using ValueType = ???
		using ValueType = int32;
	};

	template <>
	struct PropertyInfo<FDelegateProperty>
	{
		using ValueType = FScriptDelegate*;
	};

	template <>
	struct PropertyInfo<FMulticastDelegateProperty>
	{
		using ValueType = FMulticastScriptDelegate*;
	};
}