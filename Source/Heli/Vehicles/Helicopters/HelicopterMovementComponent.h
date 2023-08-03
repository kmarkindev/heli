﻿// Fill out your copyright notice in the Description page of Project Settings.

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSpeed { USpeedConversionsLibrary::KmhToCms(260.f) };
	
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
	float ForwardAccelerationScaleDefault { 1.2f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Backward")
	TObjectPtr<UCurveFloat> BackwardAccelerationScale {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Backward")
	float BackwardAccelerationScaleDefault { 0.7f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Side")
	TObjectPtr<UCurveFloat> SideAccelerationScale {};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration|Scale|Side")
	float SideAccelerationScaleDefault { 0.8f };
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

	void ApplyAccelerationsToVelocity(float DeltaTime);
	void ApplyAccelerationScaleAlongVector(FVector& BaseAcceleration, const UCurveFloat* ScaleCurve,
		float DefaultScale, const FVector& ScaleDirectionNormalized);
	void ApplyScaleToResult(FVector& InOutResult, const FVector& DeltaToApply);
	
	FVector GetHorizontalForwardVector() const;
	FVector GetHorizontalRightVector() const;
	FVector GetVerticalUpVector() const;
	
};