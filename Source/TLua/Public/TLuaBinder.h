
#pragma once

#include "TLua.hpp"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TLuaBinder.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaBinder : public UObject
{
	GENERATED_BODY()

public:
	UTLuaBinder();
//	UTLuaBinder(UObject* InOwner = nullptr, bool IsComponent = false);
	~UTLuaBinder();

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void BindActor(UObject* InOwner);

	UFUNCTION(BlueprintCallable, Category = "Lua")
	void BindComponent(UObject* InOwner);

	// template call
	template <typename ...ArgTypes>
	void TryCall(const char* Name, const ArgTypes&... Args)
	{
		TLua::Call("_lua_tcall", (void*)Owner, Name, Args...);
	}

	template <typename ...ArgTypes>
	void Call(const char* Name, const ArgTypes&... Args)
	{
		TLua::Call("_lua_call", (void*)Owner, Name, Args...);
	}

	template <typename ReturnType, typename ...ArgTypes>
	ReturnType RCall(const char* Name, const ArgTypes&... Args)
	{
		return TLua::RCall<ReturnType>("_lua_call", (void*)Owner, Name, Args...);
	}

private:
	void Bind(UObject* InOwner, bool IsComponent);
	void Unbind();

private:
	UObject* Owner;
};
