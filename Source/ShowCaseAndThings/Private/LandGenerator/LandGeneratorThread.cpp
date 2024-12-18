// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/LandGeneratorThread.h"

#include "KismetProceduralMeshLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "LandGenerator/FastNoiseLite.h"

LandGeneratorThread::LandGeneratorThread(ALandGenerator* InLandGenerator, FIntPoint sectionLocation)
	: LandGenerator(InLandGenerator), SectionLocation(sectionLocation), SectionVertexCount(InLandGenerator->SectionVertexCount), VertexSpacing(InLandGenerator->VertexSpacing)
	, Indices(InLandGenerator->Indices)
{
	//Thread = FRunnableThread::Create(this, TEXT("FLandGeneratorThread"), 0, EThreadPriority::TPri_Lowest , FPlatformAffinity::GetPoolThreadMask());
}

LandGeneratorThread::~LandGeneratorThread()
{
	if (this != nullptr)
	{
		UE_LOG(LogTemp , Warning , TEXT("this Killed"));
	}
}

uint32 LandGeneratorThread::Run()
{
	while (bInputReady)
	{
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		GenerateSectionVert(Vertices, returnVal.UVs);
		GenerateSectionTangentAndNormals(Vertices, Normals, returnVal.UVs, returnVal.Tangents);
		
		
		returnVal.Vertices = Vertices;
		returnVal.Normals = Normals;
		bInputReady = false;
	}
	return 0;
}

void LandGeneratorThread::Stop()
{
	FRunnable::Stop();
}

void LandGeneratorThread::Exit()
{
	FRunnable::Exit();
}

bool LandGeneratorThread::Init()
{
	return FRunnable::Init();
}

void LandGeneratorThread:: GenerateSectionVert(TArray<FVector>& InVertices , TArray<FVector2D>& InUvs)
{
	FVector VertOffset = FVector(SectionLocation.X * (SectionVertexCount.X -1), SectionLocation.Y * (SectionVertexCount.Y -1), 0) * VertexSpacing;
	FVector Vert;
	
	for (int32 Y = -1 ; Y <= SectionVertexCount.Y ; Y++)
	{
		for (int32 X = -1 ; X <= SectionVertexCount.X ; X++)
		{
			Vert.X = X * VertexSpacing + VertOffset.X;
			Vert.Y = Y * VertexSpacing + VertOffset.Y;
			Vert.Z = HeightNoise2D(FVector2D(Vert.X, Vert.Y));
			InVertices.Add(Vert);

			FVector2D Uv = FVector2D(( SectionVertexCount.X - 1) * SectionLocation.X + X,  (SectionVertexCount.Y - 1) * SectionLocation.Y + Y) * (VertexSpacing / 100);
			InUvs.Add(Uv);
		}
	}
}

float LandGeneratorThread::HeightNoise2D(FVector2D Position) const
{
	return LandGenerator->GetWorld()->GetSubsystem<UProceduralLandGenSubsystem>()->GroundHeightPosition(FVector2f(Position.X, Position.Y));
}

void LandGeneratorThread::GenerateSectionTangentAndNormals(TArray<FVector>& InVertices,TArray<FVector>& InNormals, TArray<FVector2D>& InUVs, TArray<FProcMeshTangent>& Tangents)
{
	int index = 0;
	
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(InVertices, Indices, InUVs, InNormals, Tangents);
	
	TArray<FVector>				OutVertices;
	TArray<FVector2D>			OutUvs;
	TArray<FVector>				OutNormals;
	TArray<FProcMeshTangent>	OutTangents;
	
	for (int32 Y = -1; Y <= SectionVertexCount.Y; ++Y)
	{
		for (int32 X = -1; X <= SectionVertexCount.X; ++X)
		{
			if (X > -1 && X < SectionVertexCount.X && Y > -1 && Y < SectionVertexCount.Y)
			{
				OutVertices.Add(InVertices[index]);
				OutUvs.Add(InUVs[index]);
				OutNormals.Add(InNormals[index]);
				OutTangents.Add(Tangents[index]);
			}
			index++;
		}
	}
	
	InVertices	= OutVertices;
	InNormals	= OutNormals;
	InUVs		= OutUvs;
	Tangents	= OutTangents;
}
