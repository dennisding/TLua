// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TLuaRootObject.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TLuaGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TLUA_API UTLuaGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual ~UTLuaGameInstanceSubsystem() {}
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UTLuaRootObject* Root;
};
