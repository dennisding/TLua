// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLua.h"
#include "Misc/Paths.h"

#include "TLua.hpp"
#include "CoreMinimal.h"

#define LOCTEXT_NAMESPACE "FTLuaModule"

DEFINE_LOG_CATEGORY(Lua);

void FTLuaModule::StartupModule()
{
	InitLua();
}

void FTLuaModule::InitLua()
{
	TLua::Init();

	TArray<FString> dirs;
	dirs.Add(TEXT("")); // current dir
	dirs.Add(TEXT("Libs/"));
	dirs.Add(TEXT("Actors/"));
	dirs.Add(TEXT("Unreal/"));
	dirs.Add(TEXT("Game/"));
	dirs.Add(TEXT("GameLibs/"));

	FString root = FPaths::ProjectContentDir() / TEXT("Script/Lua/");
	TLua::Call("_init_sys", root, dirs);

	FString initFileName = FPaths::ProjectContentDir() / TEXT("Script/Lua/init.lua");
	TLua::DoFile(initFileName);

	TLua::Call("init");
}

void FTLuaModule::ShutdownModule()
{
}

class FLuaProcessor : public FSelfRegisteringExec
{
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		FString FullCommand(Cmd);

		if (!FullCommand.StartsWith(TEXT("lua"))) {
			return false;
		}

		// send the command line to string
		TLua::Call("_lua_process_console_command", FullCommand);
		return true;
	}
};

static FLuaProcessor LuaProcessor;

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLuaModule, TLua)
