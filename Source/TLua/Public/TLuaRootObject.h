#pragma once

#include <map>
#include "Lua/lua.hpp"

#include "CoreMinimal.h"
#include "TLuaRootObject.generated.h"

class UTLuaRootObject;

class FRootTickFunction : public FTickFunction
{
public:
	FRootTickFunction();

	virtual void ExecuteTick(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent) override;

	virtual FString DiagnosticMessage() override;

public:
	UTLuaRootObject* Owner;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaCallback : public UObject
{
	GENERATED_BODY()

public:
	UTLuaCallback();
	~UTLuaCallback();

	void Bind(int AbsIndex);

	UFUNCTION()
	void Callback();

public:
};

class FCallbackMgr
{
	struct Callback
	{
		int Id;
		void* Key;

	public:
		inline Callback() : Id(0), Key(nullptr){}

		~Callback();
		void Bind();
		void Call();
	};

	using CallbackMap = std::multimap<double, Callback>;
public:
	explicit FCallbackMgr(int NextId = 100);

	// add_callback(..., duration, fun)
	int AddCallback(lua_State* State);
	void Cancel(int Handle);
	void Clear();
	void Tick(float Delta);

private:
	int NextId;
	double CurrentTime;
//	std::multimap<double, Callback> Callbacks;
	CallbackMap Callbacks;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaRootObject : public UObject
{
	GENERATED_BODY()
public:
	UTLuaRootObject();

	int AddCallback(lua_State* State);
	void CancelCallback(int Handle);
	void Activate();
	void Tick(float Delta);

private:
	FCallbackMgr CallbackMgr;
	FRootTickFunction TickFunction;
};
