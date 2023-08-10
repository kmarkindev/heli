// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HeliConversionsLibrary.generated.h"

/**
 * M - meters
 * Cm - centimeters
 * Km - kilometers
 * Ms = meters per second
 * Cms = centimeters per second
 * Kmh = kilometers per hour
 */
UCLASS()
class HELI_API UHeliConversionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float KmhToCms(float Kmh);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float CmsToKnots(float Cms);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float MsToCms(float Kms);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float CmsToKmh(float Cms);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float CmToM(float Cm);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float CmToKm(float Cm);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static float AccelMsAndMassToForce(float Acceleration, float Mass);
};
