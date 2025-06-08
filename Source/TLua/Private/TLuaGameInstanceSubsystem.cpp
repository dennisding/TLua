// Fill out your copyright notice in the Description page of Project Settings.


#include "TLuaGameInstanceSubsystem.h"

#include "CoreMinimal.h"
#include "TLua.h"
#include "TLua.hpp"


void UTLuaGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(Lua, Warning, TEXT("hello initialize"));
	TLua::Call("game_start");
}
void UTLuaGameInstanceSubsystem::Deinitialize()
{
	UE_LOG(Lua, Warning, TEXT("hello deinitialize"));
	TLua::Call("game_exit");
}
