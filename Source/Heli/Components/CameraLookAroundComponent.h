// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraLookAroundComponent.generated.h"


class USpringArmComponent;

UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class HELI_API UCameraLookAroundComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCameraLookAroundComponent();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEnabled { false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float YawLimit { 90.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PitchLimit { 90.f };

	UPROPERTY()
	TObjectPtr<APawn> OwnerPawn {};

	float CurrentPitch { 0.f };
	float CurrentYaw { 0.f };
	
public:
	virtual void InitializeComponent() override;

	UFUNCTION(BlueprintCallable)
	void AddLookAround(float X, float Y);

	UFUNCTION(BlueprintCallable)
	void SetLookAngles(float Pitch, float Yaw);

	UFUNCTION(BlueprintCallable)
	void SetIsEnabled(bool bEnable);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void UpdateControlRotation(float DeltaTime);
	
};
