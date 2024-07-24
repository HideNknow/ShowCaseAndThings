// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProceduralLandGenSubsystem.generated.h"

/**
 * 
 */

//Dynamic multicast delegate for the section change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerChangedSection , FIntPoint , SectionLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSectionGenerated , FIntPoint , SectionLocation);

class FastNoiseLite;
class UGameplayStatics;

USTRUCT(BlueprintType)
struct FNoiseGroundSettings
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Noise")
	float NoiseAmplitude = 500.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Noise")
	float NoiseScale = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Noise")
	FVector2f Seed = FVector2f(0.1f,0.1f);
	
};

UENUM(BlueprintType)
enum ENoiseFor
{
	Ground, //Noise used to generate the ground
	SeedInRange,
};

UENUM(BlueprintType)
enum ENoiseRepartitionType
{
	Perlin,			//Soft
	PoissonDisc,	//Even Repartition
	Value,			//Cubic
};

UCLASS()
class SHOWCASEANDTHINGS_API UProceduralLandGenSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	virtual void PostInitialize() override;
	virtual void BeginDestroy() override;
	
	FNoiseGroundSettings* NoiseGroundSettings;

	TMap<ENoiseFor , FastNoiseLite*> NoiseMap;

public:
	UPROPERTY(BlueprintAssignable , Category="ProceduralLandscape")
	FOnPlayerChangedSection OnPlayerChangedSection;
	UPROPERTY(BlueprintAssignable , Category="ProceduralLandscape")
	FOnSectionGenerated OnSectionGenerated;

	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static UProceduralLandGenSubsystem* GetSubsystem(const UObject* WorldContextObject);

public :
	
	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static void SetNoiseGroundSettings(const UObject* WorldContextObject ,FNoiseGroundSettings& InNoiseGroundSettings);
	
public :
	
	FastNoiseLite* GetNoise(ENoiseFor NoiseFor)
	{
		return NoiseMap[NoiseFor];
	}

	float GroundHeightPosition(FVector2f Position);
	
	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static float GetGroundHeightPosition(const UObject* WorldContextObject , FVector2f Position);
	
	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static int iSeededRandInRange(const UObject* WorldContextObject , int InMin , int InMax , FVector2f Location);
	
	UFUNCTION(BlueprintCallable, Category="Noise", meta=(WorldContext="WorldContextObject"))
	static float fSeededRandInRange(const UObject* WorldContextObject , float InMin , float InMax , FVector2f Location );

	static float Test(FastNoiseLite NoiseSettings , FVector2f Location);
	
};
