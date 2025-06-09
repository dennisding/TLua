// Fill out your copyright notice in the Description page of Project Settings.


#include "TLuaGameInstanceSubsystem.h"

#include "CoreMinimal.h"
#include "TLua.h"
#include "TLua.hpp"


void UTLuaGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	TLua::Call("game_start", (UObject*)this->GetGameInstance());
}
void UTLuaGameInstanceSubsystem::Deinitialize()
{
	TLua::Call("game_exit");
}
