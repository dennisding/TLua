#pragma once

#include "CoreMinimal.h"
#include "TLuaRootObject.generated.h"

// DECLARE_DELEGATE(FTestDelegate);
DECLARE_DYNAMIC_DELEGATE(FTestDelegate);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaRootObject : public UObject
{
	GENERATED_BODY()
public:
	UTLuaRootObject();

	UFUNCTION()
	void SayHello();

	UPROPERTY()
	FTestDelegate Delegate;
private:
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaActorCallback : public UObject
{
	GENERATED_BODY()

public:
};