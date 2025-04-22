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
	FString root = FPaths::ProjectContentDir() / TEXT("Script/Lua/");
	TLua::Init();

	TArray<FString> dirs;
	dirs.Add(TEXT("/")); // current dir
	dirs.Add(TEXT("Libs/"));
	dirs.Add(TEXT("Actors/"));
	dirs.Add(TEXT("Common/"));

	TLua::Call("_init_sys", root, dirs);
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
