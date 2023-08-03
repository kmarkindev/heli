// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeedConversionsLibrary.h"

float USpeedConversionsLibrary::KmhToCms(float Kmh)
{
	const float Ratio = 27.7777f;
		
	return Kmh * Ratio;
}

float USpeedConversionsLibrary::CmsToKnots(float Cms)
{
	const float Ratio = 0.019438f;

	return Cms * Ratio;
}

float USpeedConversionsLibrary::MsToCms(float Kms)
{
	const float Ratio = 100.f;

	return Kms * Ratio;
}

float USpeedConversionsLibrary::CmsToKmh(float Cms)
{
	const float Ratio = 0.036f;

	return Cms * Ratio;
}
