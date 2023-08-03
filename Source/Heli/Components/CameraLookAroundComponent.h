// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraLookAroundComponent.generated.h"


class USpringArmComponent;
class UCameraComponent;

UCLASS(
	ClassGroup=(Custom),
	Blueprintable,
	meta=(BlueprintSpawnableComponent),
	HideCategories=(ComponentReplication, Replication, ComponentTick)
)
class HELI_API UCameraLookAroundComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UCameraLookAroundComponent();

protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsLookAroundEnabled { false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float YawLimit { 90.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PitchLimit { 90.f };
	
	UPROPERTY()
	TObjectPtr<APawn> OwnerPawn {};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CurrentPitch { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CurrentYaw { 0.f };
	
public:
	
	virtual void InitializeComponent() override;

	UFUNCTION(BlueprintCallable)
	void AddLookAround(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable)
	void SetLookAngles(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable)
	bool IsLookAroundEnabled() const;

	UFUNCTION(BlueprintCallable)
	void SetIsLookAroundEnabled(bool bEnable);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void UpdateControlRotation();
	
};
