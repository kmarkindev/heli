// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HelicopterMovementComponent.generated.h"

USTRUCT(BlueprintType)
struct FCollocationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CurrentCollocation { 0.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CollocationIncreaseSpeed { 0.75f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CollocationDecreaseSpeed { 0.75f };
};

USTRUCT(BlueprintType)
struct FRotationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PitchSpeed { 35.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RollSpeed { 30.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float YawSpeed { 25.f };
	
};

USTRUCT(BlueprintType)
struct FPhysicsData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	FVector Velocity {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector GravityAcceleration { 0.f, 0.f, -9.8f * 100.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Cruise")
	float AccelerationScaleBeforeCruise { 0.5f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Cruise")
	float MinVelocityToEnterCruise { 11.f * 100.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration")
	float UpDecelerationRate { 0.7f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration")
	float DownDecelerationRate { 0.7f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration")
	float ForwardDecelerationRate { 0.85f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration")
	float BackwardDecelerationRate { 0.85f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration")
	float SideDecelerationRate { 0.75f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration")
	float MaxCollocationAcceleration { 16.f * 100.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration")
	float MinCollocationAcceleration { 5.f * 100.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale")
	float UpAccelerationScale { 1.0f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale")
	float DownAccelerationScale { 1.0f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale")
	float ForwardAccelerationScale { 1.0f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale")
	float BackwardAccelerationScale { 1.0f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale")
	float SideAccelerationScale { 1.0f };
	
};

UCLASS(Blueprintable, Blueprintable, meta=(BlueprintSpawnableComponent))
class HELI_API UHelicopterMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHelicopterMovementComponent();

	UFUNCTION(BlueprintCallable)
	void SetCollocation(float NewCollocation);

	UFUNCTION(BlueprintCallable)
	void IncreaseCollocation();

	UFUNCTION(BlueprintCallable)
	void DecreaseCollocation();
	
	UFUNCTION(BlueprintCallable)
	void AddRotation(float PitchIntensity, float YawIntensity, float RollIntensity);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FPhysicsData PhysicsData {};

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FCollocationData CollocationData {};

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FRotationData RotationData {};
	
	virtual void BeginPlay() override;

private:

	float CalculateAccelerationAmountBasedOnCollocation() const;
	FVector CalculateCurrentCollocationAccelerationVector() const;

	void ApplyScaleToResult(FVector& InOutResult, const FVector& DeltaToApply);
	
	void ApplyAccelerationsToVelocity(float DeltaTime);
	void ApplyAccelerationScaleAlongVector(FVector& BaseAcceleration, float Scale, const FVector& ScaleDirection);

	void ApplyDampingToVelocity(float DeltaTime);
	void ApplyDampingAlongVector(float DeltaTime, float DampingRate, const FVector& DampingDirection);

	void ApplyVelocityToLocation(float DeltaTime, FVector& OutOldLocation, FVector& OutNewLocation);

	void RecalculateVelocityBasedOnTraveledDistance(float DeltaTime, const FVector& OldLocation, const FVector& NewLocation);

	float CalculateHorizontalVelocity() const;
	float CalculateCruiseAccelerationScale() const;
	
};
