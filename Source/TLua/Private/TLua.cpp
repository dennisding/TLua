// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLua.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "TLuaLibrary/ExampleLibrary.h"

#define LOCTEXT_NAMESPACE "FTLuaModule"

void FTLuaModule::StartupModule()
{

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
