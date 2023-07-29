// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Helicopter.generated.h"

class UHelicopterMovementComponent;

UCLASS(Blueprintable)
class HELI_API AHelicopter : public APawn
{
	GENERATED_BODY()

public:
	
	AHelicopter();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UHelicopterMovementComponent> HelicopterMovementComponent {};
	
	virtual void BeginPlay() override;

public:
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
