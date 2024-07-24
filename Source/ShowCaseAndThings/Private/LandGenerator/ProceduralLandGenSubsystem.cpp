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
	GroundNoise->SetFrequency(0.001);
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

UProceduralLandGenSubsystem* UProceduralLandGenSubsystem::GetSubsystem(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return nullptr;
	}
	return World->GetSubsystem<UProceduralLandGenSubsystem>();
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
	Subsystem->GetNoise(ENoiseFor::Ground)->SetSeed(InNoiseGroundSettings.Seed.X + InNoiseGroundSettings.Seed.Y);
	Subsystem->GetNoise(ENoiseFor::Ground)->SetFrequency(InNoiseGroundSettings.NoiseFrequency);
	Subsystem->NoiseGroundLayers.Empty();
	Subsystem->NoiseGroundLayers.Add(FNoiseLayer{InNoiseGroundSettings.NoiseAmplitude,InNoiseGroundSettings.NoiseFrequency, InNoiseGroundSettings.Seed});
	
}

void UProceduralLandGenSubsystem::AddNoiseGroundLayer(const UObject* WorldContextObject,
	FNoiseLayer& InNoiseGroundSettings)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return;
	}
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();
	Subsystem->NoiseGroundLayers.Add(InNoiseGroundSettings);
}

void UProceduralLandGenSubsystem::AddNoiseGroundLayer(const UObject* WorldContextObject,
	TArray<FNoiseLayer> &InNoiseGroundSettings)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World is null"));
		return;
	}
	UProceduralLandGenSubsystem* Subsystem = World->GetSubsystem<UProceduralLandGenSubsystem>();
	Subsystem->NoiseGroundLayers.Append(InNoiseGroundSettings);
}

float UProceduralLandGenSubsystem::GroundHeightPosition(FVector2f Position)
{

	float B = this->GetNoise(ENoiseFor::Ground)->GetNoise(
		(Position.X + this->NoiseGroundSettings->Seed.X),((Position.Y +  this->NoiseGroundSettings->Seed.Y)));

	float Noise = B;

	for (int i = 1 ; i < NoiseGroundLayers.Num(); ++i)
	{
		
		float ToAdd = this->GetNoise(ENoiseFor::Ground)->GetNoise(
			(Position.X * NoiseGroundLayers[i].NoiseFrequency + (this->NoiseGroundSettings->Seed.X + NoiseGroundLayers[i].SeedOffset.X)),
			(Position.Y * NoiseGroundLayers[i].NoiseFrequency + (this->NoiseGroundSettings->Seed.Y + NoiseGroundLayers[i].SeedOffset.Y)));

		for (int a = 0; a < NoiseGroundLayers[i].Action.Num() ; ++a)
		{
			switch (NoiseGroundLayers[i].Action[a].ActionType)
			{
			default:
					;
			case None:
					break;
			case Add:
					ToAdd += NoiseGroundLayers[i].Action[a].ActionValue;
					break;
			case Subtract: 
					ToAdd -= NoiseGroundLayers[i].Action[a].ActionValue;
				break;
			case Multiply:
					ToAdd *= NoiseGroundLayers[i].Action[a].ActionValue;
				break;
			case MultiplyByMain:
					ToAdd *= Noise;
				break;
			case MultiplyByLayer:
					if (NoiseGroundLayers.IsValidIndex(NoiseGroundLayers[i].Action[a].ActionValue))
					{
						int Index = NoiseGroundLayers[i].Action[a].ActionValue;
						ToAdd *= this->GetNoise(ENoiseFor::Ground)->GetNoise(
							(Position.X * NoiseGroundLayers[Index].NoiseFrequency + (this->NoiseGroundSettings->Seed.X + NoiseGroundLayers[Index].SeedOffset.X)),
							(Position.Y * NoiseGroundLayers[Index].NoiseFrequency + (this->NoiseGroundSettings->Seed.Y + NoiseGroundLayers[Index].SeedOffset.Y)));
					}
					else
						break;
				break;
			case Divide:
					ToAdd /= NoiseGroundLayers[i].Action[a].ActionValue;
				break;
			case RemapAboveZero:
					ToAdd = (ToAdd + 1) / 2;
				break;
			}
		}
		
		Noise = Noise + ToAdd;
	}
 	return Noise * NoiseGroundSettings->NoiseAmplitude;
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
