// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/ProceduralLandGenSubsystem.h"

#include "LandGenerator/FastNoiseLite.h"
#include "Library/PoissonDiscSampling.h"

void UProceduralLandGenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProceduralLandGenSubsystem::PostInitialize()
{
	// Check out : https://auburn.github.io/FastNoiseLite/ for demo and setting up noise
	//these params ensure an almost even repartion of result from -1 to 1
	FastNoiseLite* SeededInRange = new FastNoiseLite();
	SeededInRange->SetNoiseType(FastNoiseLite::NoiseType_Value);
	SeededInRange->SetSeed(0);
	SeededInRange->SetFrequency(1.0f); 
	NoiseMap.Add(ENoiseFor::SeedInRange , SeededInRange);
	
	FastNoiseLite* GroundNoise = new FastNoiseLite();
	GroundNoise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	GroundNoise->SetSeed(0);
	GroundNoise->SetFrequency(0.010);
	NoiseMap.Add(ENoiseFor::Ground, GroundNoise);

	
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

void UProceduralLandGenSubsystem::SetNoiseGroundSettings(const UObject* WorldContextObject,
	FNoiseGroundSettings& InNoiseGroundSettings)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return;
	}
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();
	Subsystem->NoiseGroundSettings = &InNoiseGroundSettings;
}

float UProceduralLandGenSubsystem::GroundHeightPosition(FVector2f Position)
{

	float B = this->GetNoise(ENoiseFor::Ground)->GetNoise(
		(Position.X + this->NoiseGroundSettings->Seed.X) * this->NoiseGroundSettings->NoiseScale,((Position.Y +  this->NoiseGroundSettings->Seed.Y )*  this->NoiseGroundSettings->NoiseScale));
	float Noise = this->NoiseGroundSettings->NoiseAmplitude * B;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Noise : %f"), Noise));
 	return Noise;
}

float UProceduralLandGenSubsystem::GetGroundHeightPosition(const UObject* WorldContextObject, FVector2f Position)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return -1;
	}
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();
	return Subsystem->GroundHeightPosition(Position);
}

float UProceduralLandGenSubsystem::fSeededRandInRange(const UObject* WorldContextObject,
	float InMin, float InMax, FVector2f Location)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return -1;
	}
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();

	float noiseVal = (( Subsystem->GetNoise(ENoiseFor::SeedInRange)->GetNoise(Location.X , Location.Y) + 1) /2);
	float result = InMin + (InMax - InMin) * noiseVal;
	return result;
}

float UProceduralLandGenSubsystem::Test(FastNoiseLite NoiseSettings, FVector2f Location)
{
	return NoiseSettings.GetNoise(Location.X, Location.Y);
}



int UProceduralLandGenSubsystem::iSeededRandInRange(const UObject* WorldContextObject, int InMin, int InMax,
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
	float noiseVal = abs(Subsystem->GetNoise(ENoiseFor::SeedInRange)->GetNoise(Location.X , Location.Y));
	int X = Range > 0 ? FMath::Min(( noiseVal * (float)Range),Range - 1) : 0;
	return InMin + X;
}
