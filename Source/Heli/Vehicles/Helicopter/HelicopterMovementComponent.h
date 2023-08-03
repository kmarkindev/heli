// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Heli/BFLs/SpeedConversionsLibrary.h"
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
	float MaxVelocity { USpeedConversionsLibrary::KmhToCms(250.f) };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector GravityAcceleration {0.f, 0.f, USpeedConversionsLibrary::MsToCms(-9.8f)};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Up")
	TObjectPtr<UCurveFloat> UpDecelerationRate {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Up")
	float UpDecelerationRateDefault { 1.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Down")
	TObjectPtr<UCurveFloat> DownDecelerationRate {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Down")
	float DownDecelerationRateDefault { 1.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Foward")
	TObjectPtr<UCurveFloat> ForwardDecelerationRate {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Foward")
	float ForwardDecelerationRateDefault { 1.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Backward")
	TObjectPtr<UCurveFloat> BackwardDecelerationRate {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Backward")
	float BackwardDecelerationRateDefault { 1.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Side")
	TObjectPtr<UCurveFloat> SideDecelerationRate {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Deceleration|Side")
	float SideDecelerationRateDefault { 1.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration")
	float MaxCollocationAcceleration { USpeedConversionsLibrary::MsToCms(16.f) };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration")
	float MinCollocationAcceleration { USpeedConversionsLibrary::MsToCms(5.f) };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Up")
	TObjectPtr<UCurveFloat> UpAccelerationScale {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Up")
	float UpAccelerationScaleDefault { 1.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Down")
	TObjectPtr<UCurveFloat> DownAccelerationScale {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Down")
	float DownAccelerationScaleDefault { 1.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Forward")
	TObjectPtr<UCurveFloat> ForwardAccelerationScale {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Forward")
	float ForwardAccelerationScaleDefault { 1.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Backward")
	TObjectPtr<UCurveFloat> BackwardAccelerationScale {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Backward")
	float BackwardAccelerationScaleDefault { 1.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Side")
	TObjectPtr<UCurveFloat> SideAccelerationScale {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Side")
	float SideAccelerationScaleDefault { 1.f };
	
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

	FVector GetHorizontalForwardVector() const;
	FVector GetHorizontalRightVector() const;
	FVector GetVerticalUpVector() const;

	void ApplyScaleToResult(FVector& InOutResult, const FVector& DeltaToApply);
	
	void ApplyAccelerationsToVelocity(float DeltaTime);
	void ApplyAccelerationScaleAlongVector(FVector& BaseAcceleration, const UCurveFloat* ScaleCurve, float DefaultScale, const FVector& ScaleDirection);

	void ApplyDampingToVelocity(float DeltaTime);
	void ApplyDampingAlongVector(float DeltaTime, const UCurveFloat* DampingRateCurve, float DefaultRate, const FVector& DampingDirection);

	void ApplyVelocityToLocation(float DeltaTime, FVector& OutOldLocation, FVector& OutNewLocation);

	void RecalculateVelocityBasedOnTraveledDistance(float DeltaTime, const FVector& OldLocation, const FVector& NewLocation);

	float CalculateHorizontalVelocity() const;

	void ClampVelocity();
	
};
