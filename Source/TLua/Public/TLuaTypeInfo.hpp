#pragma once

#include "Lua/lua.hpp"
#include "TLuaImp.hpp"

#include "TLuaUStructMgr.hpp"

namespace TLua
{
	// <Type, is_struct, is_actorcomponent, is_actor>
	template <typename Type, typename = void, typename = void, typename = void, typename = void>
	struct TypeInfo {};

	template <typename Type>
	struct IntegerTypeInfo
	{
		inline static void ToLua(lua_State* State, Type Value)
		{
			LuaPushInteger(State, Value);
		}

		inline static void FromLua(lua_State* State, int Index, Type& OutValue)
		{
			OutValue = (Type)LuaGetInteger(State, Index);
		}

		inline static Type FromLua(lua_State* State, int Index)
		{
			return (Type)LuaGetInteger(State, Index);
		}
	};

	template <>
	struct TypeInfo<int> : public IntegerTypeInfo<int> {};
	template <>
	struct TypeInfo<int8> : public IntegerTypeInfo<int8> {};
	template <>
	struct TypeInfo<int16> : public IntegerTypeInfo<int16> {};
	template <>
	struct TypeInfo<int64> : public IntegerTypeInfo<int64> {};

	template <>
	struct TypeInfo<uint8> : public IntegerTypeInfo<uint8> {};
	template <>
	struct TypeInfo<uint16> : public IntegerTypeInfo<uint16> {};
	template <>
	struct TypeInfo<uint32> : public IntegerTypeInfo<uint32> {};
	template <>
	struct TypeInfo<uint64> : public IntegerTypeInfo<uint64> {};

	template <>
	struct TypeInfo<bool>
	{
		inline static void FromLua(lua_State* State, int Index, bool& OutValue)
		{
			OutValue = LuaGetBool(State, Index);
		}

		inline static bool FromLua(lua_State* State, int Index)
		{
			return LuaGetBool(State, Index);
		}

		inline static void ToLua(lua_State* State, bool Value)
		{
			LuaPushBool(State, Value);
		}
	};

	template <>
	struct TypeInfo<float>
	{
		inline static void FromLua(lua_State* State, int Index, float& OutValue)
		{
			OutValue = (float)LuaGetNumber(State, Index);
		}

		inline static float FromLua(lua_State* State, int Index)
		{
			return (float)LuaGetNumber(State, Index);
		}

		inline static void ToLua(lua_State* State, float Value)
		{
			LuaPushNumber(State, Value);
		}
	};

	template <>
	struct TypeInfo<double>
	{
		inline static void FromLua(lua_State* State, int Index, double& OutValue)
		{
			OutValue = LuaGetNumber(State, Index);
		}

		inline static double FromLua(lua_State* State, int Index)
		{
			return LuaGetNumber(State, Index);
		}

		inline static void ToLua(lua_State* state, double value)
		{
			LuaPushNumber(state, value);
		}
	};

	template <>
	struct TypeInfo<void*>
	{
		inline static void FromLua(lua_State* State, int Index, void*& OutValue)
		{
			OutValue = LuaGetUserData(State, Index);
		}

		inline static void* FromLua(lua_State* State, int Index)
		{
			return LuaGetUserData(State, Index);
		}

		inline static void ToLua(lua_State* State, void* Value)
		{
			LuaPushUserData(State, Value);
		}
	};


	template <>
	struct TypeInfo<char*>
	{
		inline static void FromLua(lua_State* State, int Index, const char*& OutValue)
		{
			OutValue = LuaGetString(State, Index);
		}

		inline static const char* FromLua(lua_State* State, int Index)
		{
			return LuaGetString(State, Index);
		}

		inline static void ToLua(lua_State* State, const char* Value)
		{
			LuaPushString(State, Value);
		}
	};

	template <>
	struct TypeInfo<const char*> : public TypeInfo<char*>
	{ };

	template <size_t Size>
	struct TypeInfo<char[Size]> : public TypeInfo<char*> {};

	template <>
	struct TypeInfo<std::string>
	{
		inline static void FromLua(lua_State* State, int Index, std::string& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static std::string FromLua(lua_State* State, int Index)
		{
			size_t Size = 0;
			const char* Buffer = LuaGetLString(State, Index, Size);
			return std::string(Buffer, Size);
		}

		inline static void ToLua(lua_State* State, const std::string& Value)
		{
			LuaPushLString(State, Value.c_str(), Value.size());
		}
	};

	template <>
	struct TypeInfo<FString>
	{
		inline static void FromLua(lua_State* State, int Index, FString& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static FString FromLua(lua_State* State, int Index)
		{
			size_t Size = 0;
			const TCHAR* Buffer = (const TCHAR*)LuaGetLString(State, Index, Size);
			return FString(Size / sizeof(TCHAR), Buffer);
		}

		inline static void ToLua(lua_State* State, const FString& Value)
		{
			LuaPushLString(State, (const char*)(*Value), Value.NumBytesWithoutNull());
		}
	};

	template <>
	struct TypeInfo<FName>
	{
		inline static void FromLua(lua_State* State, int Index, FName& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static FName FromLua(lua_State* State, int Index)
		{
			size_t Size = 0;
			const TCHAR* Buffer = (const TCHAR*)LuaGetLString(State, Index, Size);
			return FName(Size / sizeof(TCHAR), Buffer);
		}

		inline static void ToLua(lua_State* State, const FName& Value)
		{
			TypeInfo<FString>::ToLua(State, Value.ToString());
		}
	};

	template <>
	struct TypeInfo<FText>
	{
		inline static void FromLua(lua_State* State, int Index, FText& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static FText FromLua(lua_State* State, int Index)
		{
			FString TmpValue;
			TypeInfo<FString>::FromLua(State, Index, TmpValue);
			return FText::FromString(TmpValue);
		}

		inline static void ToLua(lua_State* State, const FText& OutValue)
		{
			TypeInfo<FString>::ToLua(State, OutValue.ToString());
		}
	};

	template <>
	struct TypeInfo<UObject*>
	{
		inline static void FromLua(lua_State* State, int Index, UObject*& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static UObject* FromLua(lua_State* State, int Index)
		{
			if (!LuaIsTable(State, Index)) {
				return nullptr;
			}
			LuaGetField(State, Index, "_co");
			UObject* Result = (UObject*)LuaGetUserData(State, -1);
			LuaPop(State, 1);
			return Result;
		}

		inline static void ToLua(lua_State* State, UObject* Value)
		{
			LuaPushUserData(State, Value);
		}
	};

	template <typename ValueType>
	struct TypeInfo<TArray<ValueType>>
	{
		using Type = TArray<ValueType>;
		inline static void FromLua(lua_State* State, int Index, Type& OutValue)
		{
			LuaLen(State, Index);
			int Size = TypeInfo<int>::FromLua(State, -1);
			LuaPop(State);

			OutValue.Reserve(Size);
			for (int Index = 0; Index < Size; ++Index) {
				LuaGetI(State, Index, Index + 1);
				OutValue.Emplace(TypeInfo<ValueType>::FromLua(State, -1));
				LuaPop(State);
			}
		}

		inline static Type FromLua(lua_State* State, int Index)
		{
			Type TmpValue;
			FromLua(State, Index, TmpValue);
			return TmpValue;
		}

		inline static void ToLua(lua_State* State, const Type& Values)
		{
			LuaNewTable(State);
			for (int Index = 0; Index < Values.Num(); ++Index) {
				TypeInfo<ValueType>::ToLua(State, Values[Index]);
				LuaSetI(State, -2, Index + 1);
			}
		}
	};

	template <typename Type>
	struct TypeInfo<Type, std::enable_if_t<std::is_enum_v<Type>, void>>
	{
		inline static void FromLua(lua_State* State, int Index, Type& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static Type FromLua(lua_State* State, int Index)
		{
			if (LuaIsTable(State, Index)) {
				LuaGetField(State, Index, "_iv");
				Type Result = (Type)LuaGetInteger(State, Index);
				LuaPop(State);
				return Result;
			}
			else {
				return (Type)LuaGetInteger(State, Index);
			}
		}

		inline static void ToLua(lua_State* State, const Type& Value)
		{
			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_enum");
			LuaPushUserData(State, (void*)StaticEnum<Type>());
			LuaPushInteger(State, (int)Value);
			LuaPCall(State, 3, 1);
		}
	};
	
	// match the struct type
	// only Struct type have the method StaticStruct
	template <typename Type>
	struct TypeInfo<Type, std::void_t<decltype(TBaseStructure<Type>::Get())>>
	{
	public:
		inline static void FromLua(lua_State* State, int Index, Type& OutValue)
		{
			if (!LuaIsTable(State, Index)) {
				return;
			}

			StructTypeProcessor* Processor = UStructProcessorMgr::Get<Type>();
			Processor->FromLua(State, Index, OutValue);
		}

		inline static Type FromLua(lua_State* State, int Index)
		{
			Type TmpValue;
			FromLua(State, Index, TmpValue);

			return TmpValue;
		}

		inline static void ToLua(lua_State* State, const Type& Value)
		{
			StructTypeProcessor* Processor = UStructProcessorMgr::Get<Type>();
			Processor->ToLua(State, &Value);
		}
	};


	template <typename Type>
	struct TypeInfo<Type, void, void,
		std::void_t<std::enable_if_t<std::is_base_of_v<UActorComponent, std::remove_pointer_t<Type>>>>>
	{

		inline static void FromLua(lua_State* State, int Index, Type& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static std::remove_pointer_t<Type>* FromLua(lua_State* State, int Index)
		{
			LuaGetField(State, Index, "_co");

			std::remove_pointer_t<Type>*  Result = (std::remove_pointer_t<Type>*)LuaGetUserData(State, -1);
			LuaPop(State, 1);
			return Result;
		}

		inline static void ToLua(lua_State* State, const Type& Value)
		{
			//LuaNewTable(State);
			//TypeInfo<void*>::ToLua(State, Value->GetClass());
			//LuaSetField(State, -2, "_ct");
			//TypeInfo<void*>::ToLua(State, Value);
			//LuaSetField(State, -2, "_co");
			//LuaGetGlobal(State, "_lc"); // lua component
			//LuaSetMetatable(State, -2);
			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_com");
			LuaPushUserData(State, Value);
			LuaPushUserData(State, Value->GetClass());
			LuaPCall(State, 3, 1);
		}
	};

	// Actor and it's child class
	template <typename Type>
	struct TypeInfo<Type, void, void,
		std::void_t<std::enable_if_t<std::is_base_of_v<AActor, std::remove_pointer_t<Type>>>>>
	{

		inline static void FromLua(lua_State* State, int Index, Type& OutValue)
		{
			OutValue = FromLua(State, Index);
		}

		inline static std::remove_pointer_t<Type>* FromLua(lua_State* State, int Index)
		{
			LuaGetField(State, Index, "_co");
			std::remove_pointer_t<Type>* Result = (std::remove_pointer_t<Type>*)LuaGetUserData(State, -1);
			LuaPop(State, 1);

			return Result;
		}

		inline static void ToLua(lua_State* State, const Type& Value)
		{
			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_obj");
			LuaPushUserData(State, Value);
			LuaPushUserData(State, Value->GetClass());
			LuaPCall(State, 3, 1);
		}
	};

	template <>
	struct TypeInfo<FWeakObjectPtr>
	{
		inline static void FromLua(lua_State* State, int Index, FWeakObjectPtr& OutValue)
		{
			OutValue = TypeInfo<UObject*>::FromLua(State, Index);
		}

		inline static FWeakObjectPtr FromLua(lua_State* State, int Index)
		{
			return TypeInfo<UObject*>::FromLua(State, Index);
		}

		inline static void ToLua(lua_State* State, const FWeakObjectPtr& Value)
		{
			TypeInfo<UObject*>::ToLua(State, Value.Get());
		}
	};
}