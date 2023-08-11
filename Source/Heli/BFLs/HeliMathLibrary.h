#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HeliMathLibrary.generated.h"

/**
 * 
 */
UCLASS()
class HELI_API UHeliMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	static float GetRotationAroundAxis(const FRotator& Rotator, const FVector& Axis);

	UFUNCTION(BlueprintCallable)
	static void SetRotationAroundAxis(FRotator& Rotator, const FVector& Axis, float Angle);

	UFUNCTION(BlueprintCallable)
	static void ClampVelocityAroundAxis(FRotator& Rotator, const FVector& Axis, float Min, float Max);
	
};
