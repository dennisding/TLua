// Fill out your copyright notice in the Description page of Project Settings.

#include "TLuaScriptComponent.h"

#include "TLua.hpp"

// Sets default values for this component's properties
UTLuaScriptComponent::UTLuaScriptComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}



// Called every frame
void UTLuaScriptComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UTLuaScriptComponent::OnRegister()
{
	Super::OnRegister();

	AActor* Actor = GetOwner();
	if (!Actor) {
		return;
	}

	
	TLua::Call("_lua_bind_obj", (void*)Actor);
}
void UTLuaScriptComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UTLuaScriptComponent::BeginPlay()
{
	Super::BeginPlay();
}
void UTLuaScriptComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UTLuaScriptComponent::OnUnregister()
{
	Super::OnUnregister();

	AActor* Actor = GetOwner();
	if (!Actor) {
		return;
	}

	TLua::Call("_lua_unbind_obj", (void *)Actor);
//	TLua::Call("_lua_silent_call_method", "_lua_unbind", (void*)Actor);
}
