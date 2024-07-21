// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "ProceduralMeshComponent.h"
#include "LandGenerator/LandGenerator.h"
#include "LandGenerator/LandGeneratorThread.h"

/**
 * 
 */
class SHOWCASEANDTHINGS_API LandGeneratorThread : FRunnable
{
public:
	LandGeneratorThread(ALandGenerator* InLandGenerator, FIntPoint sectionLocation);
	~LandGeneratorThread();

private:
	FRunnableThread* Thread;
	ALandGenerator* LandGenerator;
public:
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	virtual bool Init() override;
	
	FProceduralMeshThings returnVal;

	bool bInputReady = false;
	FIntPoint SectionLocation;
	
protected:
	FIntPoint SectionVertexCount;
	float VertexSpacing;
	TArray<int> Indices;



	bool bIsWritingResult = false;

	void GenerateSectionVert(TArray<FVector>& InVertices , TArray<FVector2D>& InUvs);
	float HeightNoise2D(FVector2D Position) const;
	void GenerateSectionTangentAndNormals(TArray<FVector>& InVertices, TArray<FVector>& InNormals, TArray<FVector2D>& InUVs, TArray<FProcMeshTangent>& Tangents);
};
