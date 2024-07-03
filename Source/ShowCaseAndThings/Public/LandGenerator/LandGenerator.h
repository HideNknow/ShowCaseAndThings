// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "LandGenerator.generated.h"

class LandGeneratorThread;

USTRUCT(BlueprintType)
struct FProceduralMeshThings
{
	GENERATED_BODY()

	FProceduralMeshThings()
	{
		Vertices = TArray<FVector>();
		Normals = TArray<FVector>();
		UVs = TArray<FVector2D>();
		Colors = TArray<FColor>();
		Tangents = TArray<FProcMeshTangent>();
	}
	
	TArray<FVector>				Vertices;
	TArray<FVector>				Normals;
	TArray<FVector2D>			UVs;
	TArray<FColor>				Colors;
	TArray<FProcMeshTangent>	Tangents;
};

UCLASS()
class SHOWCASEANDTHINGS_API ALandGenerator : public AActor
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	ALandGenerator();

	UPROPERTY( BlueprintReadWrite , EditAnywhere , Category = "Settings | Tile")
	float SectionSize = 500.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Tile")
	int SectionVertexCount = 10;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Settings | Tile")
	float VertexSpacing;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Tile")
	FIntPoint SectionCount = FIntPoint(2, 2);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Noise")
	FVector2f Seed = FVector2f(0.1f,0.1f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Noise")
	float NoiseAmplitude = 500.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings | Noise")
	float NoiseScale = 0.0003f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Land")
	UProceduralMeshComponent* LandMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Land")
	UMaterialInterface* LandMaterial;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land")
	float SectionReplaceDistance;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land")
	int MaxNumberOfSections;

	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Land | Settings | Generation")
	bool bUpdateAllThreadAtOnce = false; // if true we will update all threads at once every frame

	UPROPERTY()
	int LastWorkingThreadChecked = -1;
	
	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Land | Settings | Generation")
	bool bFastFindSectionToReplace = true; //if true we will use the closest section to the furthest minimal distance section to be replaced when creating a new one.
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION(Category = "Land Generation")
	void GenerateSectionIndices();
	
	UFUNCTION(Category = "Land Generation")
	float HeightNoise2D(FVector2D Position) const;

	int LastSectionIndex;
	
	UFUNCTION(BlueprintCallable , meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils")
	FIntVector GetFurthestSectionIndex(FVector2f Location); FIntVector GetFurthestSectionIndex(FVector Location); //return the furthest section index from a location

	UFUNCTION(BlueprintCallable , meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils")
	TArray<FIntPoint> GetSectionsInRadius(FVector2f Location);
	
	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FVector2f GetSectionCenterLocation(FIntPoint SectionLocation);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FIntPoint GetSectionByLocation(FVector2f ActorLocation); FIntPoint GetSectionByLocation(FVector ActorLocation);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	int SectionDistance(FIntPoint Section1 , FIntPoint Section2);
	
	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	bool IsSectionInBounds(FIntPoint SectionLocation);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FIntPoint GetPlayerTile();

	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//OverrideBeginDestroy
	virtual void BeginDestroy() override;

	//base indices don't need to be recalculated 
	TArray<int32> Indices;
	TArray<int32> FixedIndices;

	

	//////////////////////////////Threading//////////////////////////////
	
public:

	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch")
	void GenerateSectionAsync();

	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch")
	void AddSectionToGenerate(FIntPoint SectionLocation);

	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch" , meta = (AllowPrivateAccess = "true") , BlueprintPure)
	bool IsSectionInGeneration(FIntPoint SectionLocation);
	
	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch" , meta = (AllowPrivateAccess = "true") , BlueprintPure)
	bool CanGenerateSection(FIntPoint SectionLocation);
	
public:
	UPROPERTY(BlueprintReadWrite , EditDefaultsOnly , Category = "Land Generation Asynch")
	int MaxThreadNumber = 4;
	int NumberOfThreads = 0;
	
private:
	TArray<LandGeneratorThread*> ThreadArray; //All threads working or not
	TArray<LandGeneratorThread*> FreeThread; //All threads that are not working
	TArray<LandGeneratorThread*> WorkingThreads; //All threads that are not working
	
	TMap<FIntPoint , LandGeneratorThread*> InGenerationMap;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land Generation" , meta = (AllowPrivateAccess = "true"))
	TArray<FIntPoint> SectionsToGenerate;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land Generation" , meta = (AllowPrivateAccess = "true"))
	TMap<FIntPoint , int> GeneratedSection;

#pragma region Debug
	UFUNCTION(BlueprintCallable , Category = "Land Generation | Debug")
	void DrawDebugSection(FIntPoint SectionLocation ,float LifeTime = 0.1f , FColor Color = FColor::Red);
#pragma endregion
	
};
