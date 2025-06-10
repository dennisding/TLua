#pragma once

#include "CoreMinimal.h"
#include "TLuaRootObject.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TLUA_API UTLuaRootObject : public UObject
{
	GENERATED_BODY()
public:
	UTLuaRootObject();

	UFUNCTION()
	void SayHello();
};
