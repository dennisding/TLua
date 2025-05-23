// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Lua/lua.hpp"
#include "TLuaImp.hpp"
#include "TLuaTypes.hpp"
#include "TLuaCall.hpp"
#include "TLuaProperty.hpp"

#include "utility"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TLuaScriptComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TLUA_API UTLuaScriptComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTLuaScriptComponent();

	template <typename ...ArgTypes>
	void Call(const char* Name, const ArgTypes&... Args)
	{
		AActor* Owner = GetOwner();
		if (!Owner) {
			return;
		}

		TLua::Call("_lua_call", (void*)Owner, Name, Args...);
	}

	template <typename ReturnType, typename ...ArgTypes>
	ReturnType RCall(const char* Name, const ArgTypes&... Args)
	{
		AActor* Owner = GetOwner();
		if (!Owner) {
			return ReturnType();
		}

		return TLua::RCall<ReturnType>("_lua_call", (void*)Owner, Name, Args...);
	}

	virtual void BindOwner(AActor* Owner);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
