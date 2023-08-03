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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSpeed { USpeedConversionsLibrary::KmhToCms(250.f) };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration")
	float MaxCollocationAcceleration { USpeedConversionsLibrary::MsToCms(16.f) };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Acceleration")
	float MinCollocationAcceleration { USpeedConversionsLibrary::MsToCms(5.f) };
	
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

	void ApplyLinearMovement(float DeltaTime);
	
};
