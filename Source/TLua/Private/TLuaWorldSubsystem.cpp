// Fill out your copyright notice in the Description page of Project Settings.


#include "TLuaWorldSubsystem.h"

#include "TLua.hpp"

void UTLuaWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	TLua::Call("change_world",  (UObject*) & InWorld);
}