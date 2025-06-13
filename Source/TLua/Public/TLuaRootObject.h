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
		inline Callback() : Key(nullptr){}
		//Callback(const Callback&) = delete;
		//Callback(const Callback&&) = delete;
		//void operator=(const Callback&) = delete;
		//void operator=(const Callback&&) = delete;

		~Callback();
		void Bind();
		void Call();
	};
public:
	explicit FCallbackMgr(int NextId = 100);

	// add_callback(..., duration, fun)
	int AddCallback(lua_State* State);
	void Clear();
	void Tick(float Delta);

private:
	int NextId;
	double CurrentTime;
	std::multimap<double, Callback> Callbacks;
};

// DECLARE_DELEGATE(FTestDelegate);
DECLARE_DYNAMIC_DELEGATE(FTestDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOneDelegate, int, IntValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOneMultiDelegate, int, IntValue);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaRootObject : public UObject
{
	GENERATED_BODY()
public:
	UTLuaRootObject();

	int AddCallback(lua_State* State);
	void Activate();
	void Tick(float Delta);

public:
	// test code gose here
	UFUNCTION()
	void SayHello();

	UFUNCTION()
	void OneReturn(int IntValue);

	UPROPERTY()
	FTestDelegate Delegate;

	UPROPERTY()
	FOneDelegate OneDelegate;

	UPROPERTY()
	FOneMultiDelegate OneMultiDelegate;
private:
	FCallbackMgr CallbackMgr;
	FRootTickFunction TickFunction;
};
