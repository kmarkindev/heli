// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpeedConversionsLibrary.generated.h"

/**
 * Ms = meters per second
 * Cms = centimeters per second
 * Kmh = kilometers per hour
 */
UCLASS()
class HELI_API USpeedConversionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	static float KmhToCms(float Kmh);

	UFUNCTION(BlueprintCallable)
	static float CmsToKnots(float Cms);

	UFUNCTION(BlueprintCallable)
	static float MsToCms(float Kms);

	UFUNCTION(BlueprintCallable)
	static float CmsToKmh(float Cms);
};
