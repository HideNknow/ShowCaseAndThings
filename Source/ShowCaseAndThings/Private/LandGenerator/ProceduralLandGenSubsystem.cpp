// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/ProceduralLandGenSubsystem.h"

#include "LandGenerator/FastNoiseLite.h"
#include "Library/PoissonDiscSampling.h"

void UProceduralLandGenSubsystem::PostInitialize()
{
	FastNoiseLite* VegetationNoise = new FastNoiseLite();
	VegetationNoise->SetNoiseType(FastNoiseLite::NoiseType_Value);
	VegetationNoise->SetSeed(0);
	VegetationNoise->SetFrequency(1.0f);
	NoiseMap.Add(Vegetation, VegetationNoise);
	Super::PostInitialize();
}

void UProceduralLandGenSubsystem::BeginDestroy()
{
	for (auto Element : NoiseMap)
	{
		delete Element.Value;
	}
	Super::BeginDestroy();
}

void UProceduralLandGenSubsystem::SetNoiseGroundSettings(FNoiseSettings settings)
{
	NoiseGroundSettings = settings;
}

void UProceduralLandGenSubsystem::SetNoiseVegetationSettings(FNoiseSettings settings)
{
	NoiseVegetationSettings = settings;
}

FNoiseSettings UProceduralLandGenSubsystem::GetNoiseSettings(ENoiseFor NoiseFor) const
{
	switch (NoiseFor)
	{
		case Ground:
			return NoiseGroundSettings;
		case Vegetation:
			return NoiseVegetationSettings; 
	}
	return NoiseGroundSettings;
}

float UProceduralLandGenSubsystem::fSeededRandInRange(const UObject* WorldContextObject, ENoiseFor NoiseForWhat,
	float InMin, float InMax, FVector2f Location)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return -1;
	}
	float noiseVal = (( Subsystem->GetNoise(ENoiseFor::Vegetation)->GetNoise(Location.X , Location.Y) + 1) /2);
	float result = InMin + (InMax - InMin) * noiseVal;
	return result;
}

int UProceduralLandGenSubsystem::iSeededRandInRange(const UObject* WorldContextObject,ENoiseFor NoiseForWhat, int InMin, int InMax,
                                                   FVector2f Location)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return -1;
	}
	const int32 Range = (InMax - InMin) + 1;
	float noiseVal = abs(Subsystem->GetNoise(NoiseForWhat)->GetNoise(Location.X , Location.Y));
	int X = Range > 0 ? FMath::Min(( noiseVal * (float)Range),Range - 1) : 0;
	return InMin + X;
}
