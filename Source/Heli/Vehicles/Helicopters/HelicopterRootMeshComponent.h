// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "HelicopterRootMeshComponent.generated.h"

UCLASS(
	ClassGroup=(Custom),
	HideCategories=(Physics, ComponentTick, Collision, Deformer, Rendering,
		Navigation, ComponentReplication, Activation, Cooking, Replication),
	meta=(BlueprintSpawnableComponent)
)
class HELI_API UHelicopterRootMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	
	inline static FName HelicopterMeshSkeletonCameraSocketName { TEXT("CameraSocket") };
};
