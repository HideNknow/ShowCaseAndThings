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
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION(Category = "Land Generation")
	void GenerateSectionVert(FInt32Vector2 SectionLocation , TArray<FVector> &Vertices, TArray<FVector2D> &Uvs);

	UFUNCTION(Category = "Land Generation")
	void GenerateSectionIndices(TArray<int32> &Indices);

	UFUNCTION(Category = "Land Generation")
	void GenerateSectionTangentAndNormals(TArray<FVector>& InVertices, TArray<FVector>& InNormals, TArray<FVector2D>& InUVs, TArray<FProcMeshTangent>& Tangents);
	
	UFUNCTION(Category = "Land Generation")
	float HeightNoise2D(FVector2D Position) const;

	int LastSectionIndex;
	
	UFUNCTION(BlueprintCallable , meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils")
	FIntVector GetFurthestSectionIndex(FVector2f Location); //return the furthest section index from a location

	UFUNCTION(BlueprintCallable , meta = (AllowPrivateAccess = "true") , Category = "Land Generation | Utils")
	TArray<FIntPoint> GetSectionsInRadius(FVector2f Location);
	
	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FVector2f GetSectionCenterLocation(FIntPoint SectionLocation);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	FIntPoint GetSectionByLocation(FVector2f ActorLocation);

	UFUNCTION(BlueprintCallable , BlueprintPure , Category = "Land Generation | Utils")
	bool IsSectionInBounds(FIntPoint SectionLocation);
	
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
	
	TArray<FIntPoint> SectionsToGenerate;

	UPROPERTY(BlueprintReadOnly , VisibleAnywhere , Category = "Land Generation" , meta = (AllowPrivateAccess = "true"))
	TMap<FIntPoint , int> GeneratedSection;

};
