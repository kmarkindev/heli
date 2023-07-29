// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraLookAroundComponent.h"

#include "Kismet/KismetMathLibrary.h"

UCameraLookAroundComponent::UCameraLookAroundComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UCameraLookAroundComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCameraLookAroundComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	OwnerPawn = Cast<APawn>(GetOwner());
	if(!OwnerPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: can't get pawn pointer due to cast failure"), UE_SOURCE_LOCATION);
	}
}

void UCameraLookAroundComponent::AddLookAround(float X, float Y)
{
	CurrentYaw = UKismetMathLibrary::FClamp(CurrentYaw + X, -YawLimit, YawLimit);
	CurrentPitch = UKismetMathLibrary::FClamp(CurrentPitch + Y, -PitchLimit, PitchLimit);
}

void UCameraLookAroundComponent::SetLookAngles(float Pitch, float Yaw)
{
	CurrentYaw = UKismetMathLibrary::FClamp(Yaw, -YawLimit, YawLimit);
	CurrentPitch = UKismetMathLibrary::FClamp(Pitch, -PitchLimit, PitchLimit);
}

void UCameraLookAroundComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(!OwnerPawn || !OwnerPawn->Controller)
		return;

	// Create local rotation (local for actor)
	FRotator LocalRotation {CurrentPitch, CurrentYaw, 0.f};

	// Transform rotation from actor local space to world space
	FTransform InversedOwnerTransform = OwnerPawn->GetActorTransform().Inverse();
	FRotator FinalRotation = UKismetMathLibrary::InverseTransformRotation(InversedOwnerTransform, LocalRotation);
	
	OwnerPawn->Controller->SetControlRotation(FinalRotation);
}
