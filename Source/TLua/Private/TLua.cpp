// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLua.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "TLuaLibrary/ExampleLibrary.h"

#include "Misc/FileHelper.h"

#include "TLua.hpp"

#define LOCTEXT_NAMESPACE "FTLuaModule"

DEFINE_LOG_CATEGORY(Lua);

void FTLuaModule::StartupModule()
{
	TLua::Init();

	TArray<FString> dirs;
	dirs.Add(TEXT("")); // current dir
	dirs.Add(TEXT("Libs/"));
	dirs.Add(TEXT("Actors/"));
	dirs.Add(TEXT("Common/"));

	FString root = FPaths::ProjectContentDir() / TEXT("Script/Lua/");
	//FString root = TEXT("/Game/Script/Lua/");
	TLua::Call("_init_sys", root, dirs);

	FString initFileName = FPaths::ProjectContentDir() / TEXT("Script/Lua/init.lua");
	TLua::DoFile(initFileName);
//	TLua::Call("init");
}

void FTLuaModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(ExampleLibraryHandle);
	ExampleLibraryHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLuaModule, TLua)
