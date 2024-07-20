// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProceduralLandGenSubsystem.generated.h"

/**
 * 
 */

class FastNoiseLite;
class UGameplayStatics;

USTRUCT(BlueprintType)
struct FNoiseSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Noise")
	FVector2f Seed = FVector2f(0.1f,0.1f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Noise")
	float NoiseAmplitude = 500.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Noise")
	float NoiseScale = 0.0003f;
	
};

UENUM(BlueprintType)
enum ENoiseFor
{
	Ground,
	Vegetation
};

UCLASS()
class SHOWCASEANDTHINGS_API UProceduralLandGenSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	virtual void PostInitialize() override;
	virtual void BeginDestroy() override;
	

	FNoiseSettings NoiseGroundSettings;
	FNoiseSettings NoiseVegetationSettings;

	void SetNoiseGroundSettings(FNoiseSettings settings);
	void SetNoiseVegetationSettings(FNoiseSettings settings);
	FNoiseSettings GetNoiseSettings(ENoiseFor NoiseFor) const;

	TMap<ENoiseFor , FastNoiseLite*> NoiseMap;

public :
	FastNoiseLite* GetNoise(ENoiseFor NoiseFor)
	{
		return NoiseMap[NoiseFor];
	}

	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static int iSeededRandInRange(const UObject* WorldContextObject , ENoiseFor NoiseForWhat, int InMin , int InMax , FVector2f Location);
	
	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static float fSeededRandInRange(const UObject* WorldContextObject , ENoiseFor NoiseForWhat , float InMin , float InMax , FVector2f Location );
	
};
