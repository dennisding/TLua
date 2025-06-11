// Fill out your copyright notice in the Description page of Project Settings.


#include "TLuaGameInstanceSubsystem.h"

#include "CoreMinimal.h"
#include "TLua.h"
#include "TLua.hpp"


void UTLuaGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Root = NewObject<UTLuaRootObject>(this->GetGameInstance());
	Root->AddToRoot();
	TLua::Call("game_start", (UObject*)Root);
}
void UTLuaGameInstanceSubsystem::Deinitialize()
{
	TLua::Call("game_exit");
	Root->RemoveFromRoot();
	Root = nullptr;
}
