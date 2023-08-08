// Fill out your copyright notice in the Description page of Project Settings.


#include "HeliMathLibrary.h"

#include "Kismet/KismetMathLibrary.h"

float UHeliMathLibrary::GetRotationAroundAxis(const FRotator& Rotator, const FVector& Axis)
{
	const FQuat Quat = Rotator.Quaternion();

	return FMath::RadiansToDegrees(Quat.GetTwistAngle(Axis));
}

void UHeliMathLibrary::SetRotationAroundAxis(FRotator& Rotator, const FVector& Axis, float Angle)
{
	const float CurrentRotation = GetRotationAroundAxis(Rotator, Axis);
	const float AngleDelta = Angle - CurrentRotation;
	
	const FRotator DeltaRotation = UKismetMathLibrary::RotatorFromAxisAndAngle(
		Axis,
		AngleDelta
	);

	Rotator = UKismetMathLibrary::ComposeRotators(Rotator, DeltaRotation);
}

void UHeliMathLibrary::ClampVelocityAroundAxis(FRotator& Rotator, const FVector& Axis, float Min, float Max)
{
	const float Current = GetRotationAroundAxis(Rotator, Axis);
	const float Target = FMath::Clamp(Current, Min, Max);

	SetRotationAroundAxis(Rotator, Axis, Target);
}
