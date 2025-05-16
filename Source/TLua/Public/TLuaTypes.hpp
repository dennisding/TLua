
#pragma once

#include <string>
#include <vector>
#include <map>
#include <type_traits>

#include "TLua.hpp"
#include "TLuaImp.hpp"
#include "TLuaCall.hpp"
#include "TLuaCppLua.hpp"
#include "TLuaProperty.hpp"
#include "Lua/lua.hpp"

namespace TLua
{
	template <typename Type>
	inline Type PopValue(lua_State* state);

	// <Type, is_struct, is_actorcomponent, is_actor>
	template <typename Type, typename = void, typename = void, typename = void>
	struct TypeInfo {};

	// auto convert
	template <typename Type>
	inline Type AutoConverter(Type value) { return value; }

	template <size_t Size>
	inline char* AutoConverter(const char(&input)[Size]) { return input; }

	inline double AutoConverter(float value) { return 0.0; }

	template <typename Type>
	auto GetValue(lua_State* state, int index)
	{
		using RealType = decltype(AutoConverter(Type()));
		return (RealType)TypeInfo<RealType>::GetValue(state, index);
	}

	template <typename Type>
	inline void PushValue(lua_State* state, const Type& value)
	{
		using RealType = decltype(AutoConverter(value));
		// TypeInfo<RealType>::PushValue(state, value);
		TypeInfo<RealType>::PushValue(state, value);
	}

	template <typename Type>
	inline void SetTableByName(lua_State* State, int Index, const char* Name, const Type& Value)
	{
		int AbsIndex = LuaAbsIndex(State, Index);
		LuaPushString(State, Name); // name
		PushValue(State, Value);	// name, value
		LuaSetTable(State, AbsIndex);
	}

	template <typename Type>
	inline Type GetTableByName(lua_State* State, int Index, const char* Name)
	{
		int AbsIndex = LuaAbsIndex(State, Index);
		LuaPushString(State, Name);
		LuaGetTable(State, AbsIndex);
		return PopValue<Type>(State);
	}

	template <typename Type>
	inline void PushValues(lua_State* state, const Type& arg)
	{
		PushValue(state, arg);
	}

	template <typename First, typename ...Last>
	inline void PushValues(lua_State* state, const First& first, const Last& ...args)
	{
		PushValue(state, first);
		PushValues(state, args...);
	}

	template <typename Type>
	inline Type PopValue(lua_State* state)
	{
		Type result = TypeInfo<Type>::GetValue(state, -1);
		LuaPop(state, 1);
		return result;
	}

	// special type template
	template <>
	struct TypeInfo<int>
	{
		inline static int GetValue(lua_State* state, int index)
		{
			return (int)LuaGetInteger(state, index);
		}

		inline static void PushValue(lua_State* state, int value)
		{
			LuaPushInteger(state, value);
		}
	};

	template <>
	struct TypeInfo<bool>
	{
		inline static bool GetValue(lua_State* State, int Index)
		{
			return LuaGetBool(State, Index);
		}

		inline static void PushValue(lua_State* State, bool Value)
		{
			LuaPushBool(State, Value);
		}
	};

	template <>
	struct TypeInfo<double>
	{
		inline static double GetValue(lua_State* state, int index)
		{
			return LuaGetNumber(state, index);
		}

		inline static void PushValue(lua_State* state, double value)
		{
			LuaPushNumber(state, value);
		}
	};


	template <>
	struct TypeInfo<char*>
	{
		inline static const char* GetValue(lua_State* state, int index)
		{
			return LuaGetString(state, index);
		}

		inline static void PushValue(lua_State* state, const char* value)
		{
			LuaPushString(state, value);
		}
	};

	template <size_t Size>
	struct TypeInfo<char[Size]>
	{
		inline static const char* GetValue(lua_State* state, int index)
		{
			return LuaGetString(state, index);
		}

		inline static void PushValue(lua_State* state, const char* value)
		{
			LuaPushString(state, value);
		}
	};


	template <>
	struct TypeInfo<std::string>
	{
		inline static std::string GetValue(lua_State* state, int index)
		{
			size_t size = 0;
			const char* buffer = LuaGetLString(state, index, size);
			return std::string(buffer, size);
		}

		inline static void PushValue(lua_State* state, const std::string& value)
		{
			LuaPushLString(state, value.c_str(), value.size());
		}
	};

	template <>
	struct TypeInfo<FString>
	{
		inline static FString GetValue(lua_State* state, int index)
		{
			size_t size = 0;
			const TCHAR* buff = (const TCHAR*)LuaGetLString(state, index, size);

			return FString(size / sizeof(TCHAR), buff);
		}

		inline static void PushValue(lua_State* state, const FString& value)
		{
			LuaPushLString(state, (const char*)(*value), value.NumBytesWithoutNull());
		}
	};

	template <>
	struct TypeInfo<FName>
	{
		inline static FName GetValue(lua_State* State, int Index)
		{
			size_t Size = 0;
			const TCHAR* Buff = (const TCHAR*)LuaGetLString(State, Index, Size);
			FName Name(Size/sizeof(TCHAR), Buff);

			return Name;
		}

		inline static void PushValue(lua_State* State, const FName& Name)
		{
			FString NameString = Name.ToString();
			TypeInfo<FString>::PushValue(State, NameString);
		}
	};

	template <>
	struct TypeInfo<void*>
	{
		inline static void* GetValue(lua_State* state, int index)
		{
			return LuaGetUserData(state, index);
		}

		inline static void PushValue(lua_State* state, void* ptr)
		{
			LuaPushUserData(state, (void*)ptr);
		}
	};

	template <>
	struct TypeInfo<UObject*>
	{
		inline static void PushValue(lua_State* State, UObject* Value)
		{
			LuaPushUserData(State, Value);
		}

		inline static UObject* GetValue(lua_State* State, int Index)
		{
			if (!LuaIsTable(State, Index)) {
				return nullptr;
			}
			LuaGetField(State, Index, "_co");
			UObject* Result = (UObject*)LuaGetUserData(State, -1);
			LuaPop(State, 1);
			return Result;
		}
	};

	template <typename ValueType>
	struct TypeInfo<TArray<ValueType>>
	{
		using Type = TArray<ValueType>;
		inline static Type GetValue(lua_State* state, int index)
		{
			// check this is a table
			int abs_index = LuaAbsIndex(state, index);

			// get table length
			LuaLen(state, abs_index);
			int table_size = PopValue<int>(state);

			Type result;
			result.Reserve(table_size);
			for (int i = 1; i <= table_size; ++i) {
				TypeInfo<int>::PushValue(state, i);
				LuaGetTable(state, abs_index);
				result.Emplace(PopValue<ValueType>(state));
			}
			return result;
		}

		inline static void PushValue(lua_State* state, const Type& values)
		{
			LuaNewTable(state);
			for (int i = 0; i < values.Num(); ++i) {
				TypeInfo<int>::PushValue(state, i + 1);
				TypeInfo<ValueType>::PushValue(state, values[i]);
				LuaSetTable(state, -3);
			}
		}
	};

	// match the struct type
	// only Struct type have the method StaticStruct
	template <typename Type>
	struct TypeInfo<Type, std::void_t<decltype(TBaseStructure<Type>::Get())>>
	{
		UStruct* GetStaticStruct()
		{
			return Type::StaticStrut();
		}
	public:
		inline static Type GetValue(lua_State* State, int index)
		{
		}

		inline static void PushValue(lua_State* State, const Type& Value)
		{
			LuaNewTable(State);
			SetTableByName(State, -1, "_ct", TBaseStructure<Type>::Get());
			// iter the values

			UStruct* StructInfo = TBaseStructure<Type>::Get();
			for (TFieldIterator<FProperty> It(StructInfo); It; ++It) {
				FProperty* Property = *It;

				FString NameString = Property->GetName();
				FTCHARToUTF8 Convert(NameString);
				const char* Name = (const char*)Convert.Get();

				std::string TmpName(Convert.Get(), Convert.Length());

				if (auto* IntProperty = CastField<FIntProperty>(Property))
				{
					SetTableByName(State, -1, TmpName.c_str(),
						PropertyInfo<FIntProperty>::GetValue(&Value, IntProperty));
				}
				else if (auto* FloatProperty = CastField<FFloatProperty>(Property))
				{
					SetTableByName(State, -1, TmpName.c_str(),
						PropertyInfo<FFloatProperty>::GetValue(&Value, FloatProperty));
				}
				else if (auto* DoubleProperty = CastField<FDoubleProperty>(Property)) 
				{
					SetTableByName(State, -1, TmpName.c_str(),
						PropertyInfo<FDoubleProperty>::GetValue(&Value, DoubleProperty));
				}
				else if (auto* StrProperty = CastField<FStrProperty>(Property))
				{
					FString OutValue;
					StrProperty->GetValue_InContainer(&Value, &OutValue);
					SetTableByName(State, -1, TmpName.c_str(), OutValue);
				}
				else {
					SetTableByName(State, -1, TmpName.c_str(), 3344);
				}
			}

			// set the metatable
			LuaGetGlobal(State, "_ls");
			LuaSetMetatable(State, -2);
		}

		inline static void PushValue(lua_State* State, const Type* Value)
		{
			PushValue(State, *Value);
		}
	};


	template <typename T>
	struct TypeInfo<T, void, 
		std::void_t<std::enable_if_t<std::is_base_of_v<UActorComponent, std::remove_pointer_t<T>>>>>
	{

		inline static T GetValue(lua_State* State, int Index)
		{
			return nullptr;
		}

		inline static void PushValue(lua_State* State, const T& Value)
		{
			LuaNewTable(State);
			SetTableByName(State, -1, "_ct", (void*)Value->GetClass());
			SetTableByName(State, -1, "_co", (void*)Value);
			LuaGetGlobal(State, "_lc");
			LuaSetMetatable(State, -2);
		}
	};

	// Actor and it's child class
	template <typename Type>
	struct TypeInfo<Type, void,
		std::void_t<std::enable_if_t<std::is_base_of_v<AActor, std::remove_pointer_t<Type>>>>>
	{

		inline static Type GetValue(lua_State* State, int Index)
		{
			return nullptr;
		}

		inline static void PushValue(lua_State* State, const Type& Value)
		{
			LuaGetGlobal(State, TLUA_TRACE_CALL_NAME);
			LuaGetGlobal(State, "_lua_get_obj");
			LuaPushUserData(State, Value);
			LuaPCall(State, 2, 1);
			//LuaNewTable(State);
			//SetTableByName(State, -1, "_ct", (void*)Value->GetClass());
			//SetTableByName(State, -1, "_co", (void*)Value);
			//LuaGetGlobal(State, "_la");
			//LuaSetMetatable(State, -2);
		}
	};
}
