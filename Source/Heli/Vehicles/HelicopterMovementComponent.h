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
	float MinCollocationAcceleration { 5.f * 100.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxCollocationAcceleration { 16.f * 100.f };
	
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
	float PitchSpeed { 0.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RollSpeed { 0.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float YawSpeed { 0.f };
	
};

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class HELI_API UHelicopterMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHelicopterMovementComponent();

	UFUNCTION(BlueprintCallable)
	void SetCollocation(float NewCollocation);

	UFUNCTION(BlueprintCallable)
	float GetCollocation() const;

	UFUNCTION(BlueprintCallable)
	void IncreaseCollocation();

	UFUNCTION(BlueprintCallable)
	void DecreaseCollocation();
	
	UFUNCTION(BlueprintCallable)
	void AddRotation(float PitchIntensity, float YawIntensity, float RollIntensity);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector Velocity {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DecelerationRate { 0.4f };

	UPROPERTY(EditAnywhere)
	FCollocationData CollocationData {};

	UPROPERTY(EditAnywhere)
	FRotationData RotationData {};
	
	virtual void BeginPlay() override;
	
};
