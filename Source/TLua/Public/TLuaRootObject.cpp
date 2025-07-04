
#include "TLuaRootObject.h"

#include <cmath>

#include "TLua.h"
#include "TLua.hpp"

FRootTickFunction::FRootTickFunction() : Owner(nullptr)
{
	this->bCanEverTick = true;
	this->bStartWithTickEnabled = true;
	this->bHighPriority = false;
	this->TickGroup = TG_PrePhysics;
}

void FRootTickFunction::ExecuteTick(
	float DeltaTime,
	ELevelTick TickType,
	ENamedThreads::Type CurrentThread,
	const FGraphEventRef& MyCompletionGraphEvent)
{
	Owner->Tick(DeltaTime);
}

FString FRootTickFunction::DiagnosticMessage()
{
	return TEXT("RootObject::Tick");
}

FCallbackMgr::Callback::~Callback()
{
	if (Key) {
		lua_State* State = TLua::GetLuaState();
		lua_pushlightuserdata(State, this);
		lua_pushnil(State);
		lua_settable(State, LUA_REGISTRYINDEX);
	}
	Key = nullptr;
}

// stack: ... fun_to_bind
void FCallbackMgr::Callback::Bind()
{
	Key = this;

	lua_State* State = TLua::GetLuaState();
	lua_pushlightuserdata(State, this);
	lua_pushvalue(State, -2);
	lua_settable(State, LUA_REGISTRYINDEX);
}

void FCallbackMgr::Callback::Call()
{
	lua_State* State = TLua::GetLuaState();
	lua_getglobal(State, TLUA_TRACE_CALL_NAME);
	lua_pushlightuserdata(State, this);
	lua_gettable(State, LUA_REGISTRYINDEX);
	lua_pcall(State, 1, 0, 0); // 不会返回错误, 错误已在lua内部处理了.
}

// callback manager
FCallbackMgr::FCallbackMgr(int InNextId) : NextId(InNextId), CurrentTime(0.)
{
}

// _add_callback(..., delta, fun)
int FCallbackMgr::AddCallback(lua_State* State)
{
	// alloc the id
	int Id = NextId;
	NextId += 1;

	// duration
	float Delta = lua_tonumber(State, -2);
	double ActivateTime = std::nextafter(CurrentTime + Delta, std::numeric_limits<double>::infinity());

	auto Iterator = Callbacks.emplace(std::make_pair(ActivateTime, Callback()));

	Iterator->second.Id = Id;
	Iterator->second.Bind();
	
	return Id;
}

void FCallbackMgr::Cancel(int Handle)
{
	for (CallbackMap::iterator It = Callbacks.begin(); It != Callbacks.end(); ++It) {
		if (It->second.Id == Handle) {
			Callbacks.erase(It);
			return;
		}
	}
}

void FCallbackMgr::Clear()
{
	Callbacks.clear();
}

void FCallbackMgr::Tick(float Delta)
{
	CurrentTime += Delta;

	// tick the fun
	auto Iterator = Callbacks.begin();
	while (Iterator != Callbacks.end()) {
		if (Iterator->first < CurrentTime) {
			// activate
			Iterator->second.Call();
			Callbacks.erase(Iterator);

			// next iterator
			Iterator = Callbacks.begin();
		}
		else {
			break; // no callback activate
		}
	}
}

UTLuaCallback::UTLuaCallback() : CallbackContext(nullptr)
{
}

UTLuaCallback::~UTLuaCallback()
{
	check(IsInGameThread());

	lua_State* State = TLua::GetLuaState();
	lua_pushlightuserdata(State, this);
	lua_pushnil(State);
	lua_settable(State, LUA_REGISTRYINDEX);
}

void UTLuaCallback::Bind(TLua::FunctionContext* InCallbackContext, int AbsIndex)
{
	CallbackContext = InCallbackContext;

	lua_State* State = TLua::GetLuaState();
	lua_pushlightuserdata(State, this);
	lua_pushvalue(State, AbsIndex);
	lua_settable(State, LUA_REGISTRYINDEX);
}

void UTLuaCallback::Callback()
{
	lua_State* State = TLua::GetLuaState();
	lua_getglobal(State, TLUA_TRACE_CALL_NAME);
	lua_pushlightuserdata(State, this);
	lua_gettable(State, LUA_REGISTRYINDEX);

	lua_pcall(State, 1, 0, 0);
}

void UTLuaCallback::ProcessEvent(UFunction* Function, void* Parameters)
{
	lua_State* State = TLua::GetLuaState();
	lua_getglobal(State, TLUA_TRACE_CALL_NAME);
	lua_pushlightuserdata(State, this);
	lua_gettable(State, LUA_REGISTRYINDEX);

	const int ExtraParameter = 1;

	CallbackContext->CallLua(ExtraParameter, Parameters);
}

UTLuaRootObject::UTLuaRootObject()
{
	TickFunction.Owner = this;
}

int UTLuaRootObject::AddCallback(lua_State* State)
{
	return CallbackMgr.AddCallback(State);
}

void UTLuaRootObject::CancelCallback(int Handle)
{
	CallbackMgr.Cancel(Handle);
}

void UTLuaRootObject::Activate()
{
	UWorld* World = GetWorld();
	if (!World) {
		return;
	}

	if (!TickFunction.IsTickFunctionRegistered()) {
		TickFunction.RegisterTickFunction(World->PersistentLevel);
	}
}

void UTLuaRootObject::Tick(float Delta)
{
	CallbackMgr.Tick(Delta);
}
