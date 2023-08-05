// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhysicsConvertionsLibrary.generated.h"

/**
 * 
 */
UCLASS()
class HELI_API UPhysicsConvertionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float AccelMsAndMassToForce(float Acceleration, float Mass);
};
