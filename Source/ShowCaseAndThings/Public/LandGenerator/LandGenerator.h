// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralLandGenSubsystem.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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

USTRUCT(BlueprintType)
struct FVegetation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Vegetation | Mesh")
	UStaticMesh* Mesh;
	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Vegetation | Mesh")
	UMaterialInterface* Material;
	
	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Vegetation | Density")
	float MinDensity;
	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Vegetation | Density")
	float MaxDensity;
	
	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Vegetation | Density")
	float MinDistance; //Minimal distances between two instances

	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Vegetation | Density")
	TEnumAsByte<ENoiseRepartitionType> RepartionType; //Noise Type To use repartition

	FVegetation()
	{
		MinDensity = 0.1f;
		MaxDensity = 0.5f;
		MinDistance = 100.0f; 
		RepartionType = ENoiseRepartitionType::Perlin;
	}
};

UCLASS()
class SHOWCASEANDTHINGS_API ALandGenerator : public AActor
{
	GENERATED_BODY()
	
public:	//Components

	UPROPERTY(BlueprintReadOnly , Category = "Land")
	UProceduralMeshComponent* LandMesh;

	UPROPERTY(BlueprintReadOnly , Category = "Land")
	UInstancedStaticMeshComponent* LandMeshInstances;
	
public:
	
	// Sets default values for this actor's properties
	ALandGenerator();
	~ALandGenerator();

	UPROPERTY( BlueprintReadWrite , EditAnywhere , Category = "Land | Tile")
	float SectionSize = 500.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Tile")
	int SectionVertexCount = 10;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land | Tile")
	float VertexSpacing;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Land | Tile")
	int ChunckRenderDistance = 1;
	int RenderBounds;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "AAAAA" , DisplayName = "Noise Settings")
	FNoiseGroundSettings NoiseGroundSettings;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Land | Rendering ")
	UMaterialInterface* LandMaterial;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land | Utils ")
	int MaxNumberOfSections;

	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Land | Thread")
	bool bUpdateAllThreadAtOnce = false; //if true we will update all threads at once every frame

	UPROPERTY(BlueprintReadWrite , EditDefaultsOnly , Category = "Land | Thread")
	int MaxThreadNumber = 4;
	
	UPROPERTY(BlueprintReadWrite , EditAnywhere , Category = "Land | Misc")
	bool bFastFindSectionToReplace = true; //if true we will use the closest section to the furthest minimal distance section to be replaced when creating a new one.
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//OverrideBeginDestroy
	virtual void BeginDestroy() override; 
	
	UFUNCTION(Category = "Land Generation")
	void GenerateSectionIndices();
	
	UFUNCTION(BlueprintCallable , meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils") //return the furthest section index from a location
	FIntVector GetFurthestSectionIndex(FVector2f Location); FIntVector GetFurthestSectionIndex(FVector Location) {return GetFurthestSectionIndex(FVector2f(Location.X, Location.Y));};

	UFUNCTION(BlueprintCallable ,BlueprintPure, meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils")
	bool IsTileOutOfChunkDistance(FIntPoint SectionLocation);
	
	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FVector2f GetSectionCenterLocation(FIntPoint SectionLocation);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FIntPoint GetSectionByLocation(FVector2f ActorLocation); FIntPoint GetSectionByLocation(FVector ActorLocation) {return GetSectionByLocation(FVector2f(ActorLocation.X, ActorLocation.Y));};

	UFUNCTION(BlueprintCallable , meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils")
	TArray<FIntPoint> GetSectionsInRadius(FVector2f Location);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	int GetDistanceToSection(FIntPoint Section1 , FIntPoint Section2);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FIntPoint GetPlayerTile();

public:	

	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch")
	void AddSectionToGenerate(FIntPoint SectionLocation);

	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch" , meta = (AllowPrivateAccess = "true") , BlueprintPure)
	bool IsSectionInGeneration(FIntPoint SectionLocation);
	
	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch" , meta = (AllowPrivateAccess = "true") , BlueprintPure)
	bool CanGenerateSection(FIntPoint SectionLocation);

	//base indices don't need to be recalculated 
	TArray<int32> Indices;
	TArray<int32> FixedIndices;
private:
	
	int LastSectionIndex;
	
	TArray<LandGeneratorThread*> ThreadArray; //All threads working or not
	TArray<LandGeneratorThread*> FreeThread; //All threads that are not working
	TArray<LandGeneratorThread*> WorkingThreads; //All threads that are not working

	int NumberOfThreads = 0;

	UPROPERTY()
	int LastWorkingThreadChecked = -1;
	
	TMap<FIntPoint , LandGeneratorThread*> InGenerationMap;
	

	UPROPERTY(BlueprintReadOnly , Category = "Land Generation" , meta = (AllowPrivateAccess = "true"))
	TArray<FIntPoint> SectionsToGenerate;

	UPROPERTY(BlueprintReadOnly , Category = "Land Generation" , meta = (AllowPrivateAccess = "true"))
	TMap<FIntPoint , int> GeneratedSection;

	UFUNCTION(BlueprintCallable , Category = "Land Generation Asynch")
	void GenerateSectionAsync(); //Assign Work to threads
	
	UFUNCTION()
	void UpdateThread(); //Iterate on threads and check if they are done then update the mesh

	UFUNCTION(BlueprintCallable , Category = "Land Generation")
	void GenerateVegetationOnSection(FIntPoint SectionLocation , FVegetation Trees);

#pragma region Debug
	UFUNCTION(BlueprintCallable , Category = "Land Generation | Debug")
	void DrawDebugSection(FIntPoint SectionLocation ,float LifeTime = 0.1f , FColor Color = FColor::Red);
#pragma endregion
	
};
