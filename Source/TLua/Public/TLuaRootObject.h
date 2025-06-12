#pragma once

#include "CoreMinimal.h"
#include "TLuaRootObject.generated.h"

// DECLARE_DELEGATE(FTestDelegate);
DECLARE_DYNAMIC_DELEGATE(FTestDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOneDelegate, int, IntValue);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaRootObject : public UObject
{
	GENERATED_BODY()
public:
	UTLuaRootObject();

	UFUNCTION()
	void SayHello();

	UFUNCTION()
	void OneReturn(int IntValue);

	UPROPERTY()
	FTestDelegate Delegate;

	UPROPERTY()
	FOneDelegate OneDelegate;
private:
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaActorCallback : public UObject
{
	GENERATED_BODY()

public:
};