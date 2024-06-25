// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/LandGenerator.h"
#include "LandGenerator/LandGeneratorThread.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ALandGenerator::ALandGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LandMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LandMesh"));
	LandMesh->SetupAttachment(this->GetRootComponent());
}

// Called when the game starts or when spawned
void ALandGenerator::BeginPlay()
{
	VertexSpacing = SectionSize /( SectionVertexCount -1);
	
	for (int i = 0; i < MaxThreadNumber; ++i)
	{
		ThreadArray.Add(new LandGeneratorThread(this, FIntPoint(0, 0)));
	}
	FreeThread = ThreadArray;

	GenerateSectionIndices(Indices);

	SectionReplaceDistance = SectionSize * ((SectionCount.X + SectionCount.Y) /2);
	MaxNumberOfSections = SectionCount.X * SectionCount.Y;
	
	Super::BeginPlay();
}

// Called every frame
void ALandGenerator::Tick(float DeltaTime)
{
	GEngine->AddOnScreenDebugMessage(-1, this->GetActorTickInterval(), FColor::Red, FString::Printf(TEXT("Free threads: %d"), FreeThread.Num()));
	GEngine->AddOnScreenDebugMessage(-1, this->GetActorTickInterval(), FColor::Red, FString::Printf(TEXT("Total threads: %d"), ThreadArray.Num()));
	GEngine->AddOnScreenDebugMessage(-1, this->GetActorTickInterval(), FColor::Red, FString::Printf(TEXT("Sections to generate: %d"), SectionsToGenerate.Num()));

	
	for (int index = WorkingThreads.Num() - 1; index >= 0; --index)
	{
		if (WorkingThreads[index]->bInputReady == false)
		{
			InGenerationMap.Remove(WorkingThreads[index]->SectionLocation);

			if (LastSectionIndex <= MaxNumberOfSections)
			{
				GeneratedSection.Add(WorkingThreads[index]->SectionLocation, LastSectionIndex);
				LandMesh->CreateMeshSection(LastSectionIndex, WorkingThreads[index]->returnVal.Vertices, FixedIndices
				, WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
				, WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents, true);
				LandMesh->SetMaterial(LastSectionIndex, LandMaterial);
				LastSectionIndex++;
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Section %d %d generated"), WorkingThreads[index]->SectionLocation.X, WorkingThreads[index]->SectionLocation.Y));
			}
			else
			{
				//Player Location
				FVector Origin = UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation();
				
				FIntVector3 FurthestSectionIndex = GetFurthestSectionIndex(FVector2f(Origin.X, Origin.Y));

				GeneratedSection.Add(WorkingThreads[index]->SectionLocation, FurthestSectionIndex.Z);
				GeneratedSection.Remove(FIntPoint(FurthestSectionIndex.X, FurthestSectionIndex.Y));
				
				LandMesh->UpdateMeshSection(FurthestSectionIndex.Z, WorkingThreads[index]->returnVal.Vertices
				, WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
				, WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents);
			}
			
			LandGeneratorThread* Thread = WorkingThreads[index];
			WorkingThreads.RemoveAt(index);
			FreeThread.Add(Thread);
		}
	}
	
	if (SectionsToGenerate.Num() > 0 && FreeThread.Num() > 0)
	{
		GenerateSectionAsync();
	}
	
	Super::Tick(DeltaTime);
}

void ALandGenerator:: GenerateSectionVert(FIntVector2 SectionLocation, TArray<FVector>& InVertices, TArray<FVector2D>& InUvs)
{

	FVector VertOffset = FVector(SectionLocation.X * (SectionVertexCount -1), SectionLocation.Y * (SectionVertexCount -1), 0) * SectionSize;
	FVector Vert;
	
	for (int32 Y = -1 ; Y <= SectionVertexCount ; Y++)
	{
		for (int32 X = -1 ; X <= SectionVertexCount ; X++)
		{
			Vert.X = X * SectionSize + VertOffset.X;
			Vert.Y = Y * SectionSize + VertOffset.Y;
			Vert.Z = HeightNoise2D(FVector2D(Vert.X, Vert.Y));
			InVertices.Add(Vert);

			FVector2D Uv = FVector2D(( SectionVertexCount- 1) * SectionLocation.X + X,  (SectionVertexCount - 1) * SectionLocation.Y + Y) * (SectionSize / 100);
			InUvs.Add(Uv);
		}
	}
}

void ALandGenerator::GenerateSectionIndices(TArray<int32>& InIndices)
{
	for (int32 Y = 0 ; Y <= SectionVertexCount ; ++Y)
	{
		for (int32 X = 0 ; X <= SectionVertexCount; ++X)
		{
			int First = X + Y * (SectionVertexCount +2);
			int Second = X + (Y + 1) * (SectionVertexCount +2);
			int Third = X + Y * (SectionVertexCount +2) + 1;
			int Fourth = X + (Y + 1) * (SectionVertexCount +2);
			int Fifth = X + (Y + 1) * (SectionVertexCount +2) + 1;
			int Sixth = X + Y * (SectionVertexCount +2) + 1;

			InIndices.Add(First);
			InIndices.Add(Second);
			InIndices.Add(Third);
			InIndices.Add(Fourth);
			InIndices.Add(Fifth);
			InIndices.Add(Sixth);
		}
	}


	for (int32 Y = 0; Y <= SectionVertexCount - 2; ++Y)
	{
		for (int32 X = 0; X <= SectionVertexCount - 2 ; ++X)
		{
			int First = SectionVertexCount * Y + X;
			int Second = First + SectionVertexCount;
			int Third = First + 1;
			int Fourth = Second ;
			int Fifth = Second + 1;
			int Sixth = Third;
			
			FixedIndices.Add(First);
			FixedIndices.Add(Second);
			FixedIndices.Add(Third);
			FixedIndices.Add(Fourth);
			FixedIndices.Add(Fifth);
			FixedIndices.Add(Sixth);
		}
	}
}

void ALandGenerator::GenerateSectionTangentAndNormals(TArray<FVector>& InVertices,TArray<FVector>& InNormals, TArray<FVector2D>& InUVs, TArray<FProcMeshTangent>& Tangents)
{
	//Calculate normals
	TArray<FVector> TileNormal;
	TArray<FProcMeshTangent> TileTangent;
	
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(InVertices , Indices , InUVs , TileNormal , TileTangent);
	
	int index = 0;
	TArray<FVector>				OutVertices;
	TArray<FVector2D>			OutUvs;
	TArray<FVector>				OutNormals;
	TArray<FProcMeshTangent>	OutTangents = Tangents;
	
	for (int32 Y = -1; Y <= SectionVertexCount; ++Y)
	{
		for (int32 X = -1; X <= SectionVertexCount; ++X)
		{
			if (X > -1 && X < SectionVertexCount && Y > -1 && Y < SectionVertexCount)
			{
				OutVertices.Add(InVertices[index]);
				OutUvs.Add(InUVs[index]);
				OutNormals.Add(TileNormal[index]);
				OutTangents.Add(TileTangent[index]);
			}
			index++;
		}
	}
	
	InVertices	= OutVertices;
	InNormals	= OutNormals;
	InUVs		= OutUvs;
	Tangents	= OutTangents;
};

float ALandGenerator::HeightNoise2D(FVector2D Position) const
{
	float Noise = NoiseAmplitude * (UKismetMathLibrary::PerlinNoise1D((Position.X + Seed.X) * NoiseScale) + UKismetMathLibrary::PerlinNoise1D((Position.Y + Seed.Y )* NoiseScale));
	return Noise;
}

FIntVector ALandGenerator::GetFurthestSectionIndex(FVector2f Location)
{
	if (GeneratedSection.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No section generated yet"));
		return FIntVector(-1,-1,-1);
	}

	
	FVector2f ClosestLocation = Location;
	FIntPoint FurthestLocation = FIntPoint();

	float FurtherDistance = 0;
	float Distance =0;
	
	
	for (auto Element : GeneratedSection)
	{
		GetSectionCenterLocation(Element.Key); // Location of tile
		Distance = FVector2f::Distance(ClosestLocation, GetSectionCenterLocation(Element.Key));
		if (Distance > FurtherDistance)
		{
			FurtherDistance = Distance;
			if (Distance >= SectionReplaceDistance)
			{
				FurthestLocation = Element.Key;		
				break;
			}
			FurthestLocation = Element.Key;	
		}
	}
	if (!GeneratedSection.Contains(FurthestLocation))
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Furthest Section %d %d not generated yet"), FurthestLocation.X, FurthestLocation.Y));
	}
	else if (Distance < SectionReplaceDistance)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Furthest Section %d %d is not far enough"), FurthestLocation.X, FurthestLocation.Y));
	}
	
	return FIntVector(FurthestLocation.X, FurthestLocation.Y, GeneratedSection[FurthestLocation]);
}

TArray<FIntPoint> ALandGenerator::GetSectionsInRadius(FVector2f Location)
{
	FIntPoint CenterLocation = GetSectionByLocation(Location);

	FIntPoint StartLocation ;
	StartLocation.X = CenterLocation.X - (SectionCount.X / 2);
	StartLocation.Y = CenterLocation.Y - (SectionCount.Y / 2);
	FIntPoint EndLocation;
	EndLocation.X = ( StartLocation.X + SectionCount.X) - 1;
	EndLocation.Y = ( StartLocation.Y + SectionCount.Y) - 1;
	
	TArray<FIntPoint> ToReturn;
	for (int Y = StartLocation.Y; Y <= EndLocation.Y; ++Y)
	{
		for (int X = StartLocation.X; X <= EndLocation.X; ++X)
		{
			ToReturn.Add(FIntPoint(X, Y));
		}
	}
	return ToReturn;

}

FVector2f ALandGenerator::GetSectionCenterLocation(FIntPoint SectionLocation)
{
	return FVector2f(SectionLocation.X * SectionSize + SectionSize / 2 , SectionLocation.Y * SectionSize + SectionSize / 2);
}

FIntPoint ALandGenerator::GetSectionByLocation(FVector2f ActorLocation)
{
	return FIntPoint(FMath::FloorToInt(ActorLocation.X / SectionSize), FMath::FloorToInt(ActorLocation.Y / SectionSize));
}

bool ALandGenerator::IsSectionInBounds(FIntPoint SectionLocation)
{
	FVector2f Origin = GetSectionCenterLocation(SectionLocation);
	float Distance =  FVector::Distance(FVector(Origin.X, Origin.Y , 0.0f),
		UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation() * FVector(1,1,0));
	return Distance <= SectionReplaceDistance;
}

void ALandGenerator::BeginDestroy()
{
	for (int i = 0; i < ThreadArray.Num(); ++i)
	{
		ThreadArray[i]->Stop();
		ThreadArray[i]->Exit();
		delete ThreadArray[i];
	}
	Super::BeginDestroy();
}

void ALandGenerator::GenerateSectionAsync()
{

	int index = FreeThread.Num() - 1;
	while (index >= 0)
	{
		int indexToProcess = SectionsToGenerate.Num() - 1;
		bool bfoundAnIndexToProcess = false;

		while (!bfoundAnIndexToProcess && indexToProcess >= 0)
		{
			if (InGenerationMap.Contains(SectionsToGenerate[indexToProcess]) || GeneratedSection.Contains(SectionsToGenerate[indexToProcess]) || !IsSectionInBounds(SectionsToGenerate[indexToProcess]))
			{
				//Log text Section is generating
				UE_LOG(LogTemp, Warning, TEXT("Section %d %d already generating of index : %d"), SectionsToGenerate[indexToProcess].X , SectionsToGenerate[indexToProcess].Y , indexToProcess);
				SectionsToGenerate.Pop();
			}
			else
			{
				bfoundAnIndexToProcess = true;
				LandGeneratorThread* Thread = FreeThread.Pop();
				WorkingThreads.Add(Thread);
				InGenerationMap.Add(SectionsToGenerate[indexToProcess], Thread);
				
				Thread->SectionLocation = SectionsToGenerate[indexToProcess];
				SectionsToGenerate.Pop();

				Thread->bInputReady = true;
				Thread->Run();

			}
			--indexToProcess ;
		}
		--index;
	}

	
}

void ALandGenerator::AddSectionToGenerate(FIntPoint SectionLocation)
{
	if (InGenerationMap.Contains(SectionLocation))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Section %d %d already generating"), SectionLocation.X, SectionLocation.Y));
		return;
	}
	else if (GeneratedSection.Contains(SectionLocation))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Section %d %d already generated"), SectionLocation.X, SectionLocation.Y));
		return;
	}
	else if (SectionsToGenerate.Contains(SectionLocation))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Section %d %d already in queue"), SectionLocation.X, SectionLocation.Y));
		return;
	}
	else
	{
		if (SectionsToGenerate.Num() >= MaxNumberOfSections)
		{
			SectionsToGenerate.Pop();
		}
		SectionsToGenerate.Add(SectionLocation);
	}
}

bool ALandGenerator::IsSectionInGeneration(FIntPoint SectionLocation)
{
	return InGenerationMap.Contains(SectionLocation);
}

bool ALandGenerator::CanGenerateSection(FIntPoint SectionLocation)
{
	return !InGenerationMap.Contains(SectionLocation) && !GeneratedSection.Contains(SectionLocation) && !SectionsToGenerate.Contains(SectionLocation);
}
