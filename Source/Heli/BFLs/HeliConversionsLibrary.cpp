// Fill out your copyright notice in the Description page of Project Settings.


#include "HeliConversionsLibrary.h"

float UHeliConversionsLibrary::KmhToCms(float Kmh)
{
	const float Ratio = 27.7777f;
		
	return Kmh * Ratio;
}

float UHeliConversionsLibrary::CmsToKnots(float Cms)
{
	const float Ratio = 0.019438f;

	return Cms * Ratio;
}

float UHeliConversionsLibrary::MsToCms(float Kms)
{
	const float Ratio = 100.f;

	return Kms * Ratio;
}

float UHeliConversionsLibrary::CmsToMs(float Cms)
{
	const float Ratio = 100.f;

	return Cms / Ratio;
}

float UHeliConversionsLibrary::CmsToKmh(float Cms)
{
	const float Ratio = 0.036f;

	return Cms * Ratio;
}

float UHeliConversionsLibrary::CmToM(float Cm)
{
	const float Ratio = 100.f;

	return Cm / Ratio;
}

float UHeliConversionsLibrary::CmToKm(float Cm)
{
	const float Ratio = 100000.f;
	
	return Cm / Ratio;
}

float UHeliConversionsLibrary::AccelMsAndMassToForce(float Acceleration, float Mass)
{
	return Acceleration * Mass;
}
