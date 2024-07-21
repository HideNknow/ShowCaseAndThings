// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/LandGenerator.h"
#include "LandGenerator/LandGeneratorThread.h"



#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "LandGenerator/ProceduralLandGenSubsystem.h"
#include "Library/PoissonDiscSampling.h"
#include "Runtime/Core/Tests/Containers/TestUtils.h"

// Sets default values
ALandGenerator::ALandGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LandMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LandMesh"));
	LandMesh->SetupAttachment(this->GetRootComponent());
	LandMesh->bUseAsyncCooking = true;

	LandMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LandMeshInstances"));
	LandMeshInstances->SetupAttachment(this->GetRootComponent());
	
	NoiseGroundSettings.Seed = FVector2f(0.1f, 0.1f);
	NoiseGroundSettings.NoiseAmplitude = 1000.0f;
	NoiseGroundSettings.NoiseScale = 1.0f;
	
}

ALandGenerator::~ALandGenerator()
{

}

// Called when the game starts or when spawned
void ALandGenerator::BeginPlay()
{
	VertexSpacing = SectionSize /( SectionVertexCount -1);
	RenderBounds = ChunckRenderDistance*2+1;
	MaxNumberOfSections = RenderBounds * RenderBounds;
	
	GenerateSectionIndices();
	for (int i = 0; i < MaxThreadNumber; ++i)
	{
		ThreadArray.Add(new LandGeneratorThread(this, FIntPoint(0, 0)));
	}
	FreeThread = ThreadArray;

	UProceduralLandGenSubsystem::SetNoiseGroundSettings(GetWorld(), NoiseGroundSettings);;
	
	this->SetActorTickEnabled(true);
	
	Super::BeginPlay();
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

void ALandGenerator::GenerateSectionIndices()
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

			Indices.Add(First);
			Indices.Add(Second);
			Indices.Add(Third);
			Indices.Add(Fourth);
			Indices.Add(Fifth);
			Indices.Add(Sixth);
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

// Called every frame
void ALandGenerator::Tick(float DeltaTime)
{
	GEngine->AddOnScreenDebugMessage(-1, this->GetActorTickInterval(), FColor::Red, FString::Printf(TEXT("Total threads: %d   Free threads: %d"), ThreadArray.Num() ,FreeThread.Num()));
	GEngine->AddOnScreenDebugMessage(-1, this->GetActorTickInterval(), FColor::Red, FString::Printf(TEXT("Sections to generate: %d"), SectionsToGenerate.Num()));
	
	UpdateThread();//Update the thread to check if the thread is done generating the section

	GenerateSectionAsync(); //Iterate over the sections to generate and check if we can assign them to workers

	//So now we need to generate vegetations and other stuffs

	
	
	Super::Tick(DeltaTime);
}

void ALandGenerator::GenerateSectionAsync()
{
	if (SectionsToGenerate.Num() <= 0 &&  FreeThread.Num() <= 0)
	{
		return;
	}
	
	int index = FreeThread.Num() - 1;
	while (index >= 0)
	{
		int indexToProcess = SectionsToGenerate.Num() - 1;
		bool bfoundAnIndexToProcess = false;

		while (!bfoundAnIndexToProcess && indexToProcess >= 0)
		{
			if (InGenerationMap.Contains(SectionsToGenerate[indexToProcess]) || GeneratedSection.Contains(SectionsToGenerate[indexToProcess]))
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

void ALandGenerator::UpdateThread()
{
	switch (bUpdateAllThreadAtOnce)
	{
	case true :
	 	for (int index = WorkingThreads.Num() - 1; index >= 0; --index) //On verifie si tout les threads sont pret en iterant sur chacun d'eux.
	 	{																//trop de threads en meme temps = trop de sections a generer en meme temps
	 		if (WorkingThreads[index]->bInputReady == false)
	 		{
	 			InGenerationMap.Remove(WorkingThreads[index]->SectionLocation);
	 			
	 			if (LastSectionIndex < MaxNumberOfSections) //Si on a pas atteint le nombre maximum de sections on ajoute la section
	 			{
	 				GeneratedSection.Add(WorkingThreads[index]->SectionLocation, LastSectionIndex);
	 				LandMesh->CreateMeshSection(LastSectionIndex, WorkingThreads[index]->returnVal.Vertices, FixedIndices
					 , WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
					 , WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents, true);
	 				LandMesh->SetMaterial(LastSectionIndex, LandMaterial);
	 				LastSectionIndex++;
	 			}
	 			else //Sinon on recreer la section la plus eloignee
	 			{
	 			    //Player Location
	 			    FVector Origin = UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation();
	 			    
	 			    FIntVector3 FurthestSectionIndex = GetFurthestSectionIndex(FVector2f(Origin.X, Origin.Y));
	            	
	 			    GeneratedSection.Remove(FIntPoint(FurthestSectionIndex.X, FurthestSectionIndex.Y));
	            	
	 			    LandMesh->ClearMeshSection(FurthestSectionIndex.Z);
	 			    LandMesh->CreateMeshSection(FurthestSectionIndex.Z, WorkingThreads[index]->returnVal.Vertices, FixedIndices
	 			    , WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
	 			    , WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents, true);
	 			    
	 			    GeneratedSection.Add(WorkingThreads[index]->SectionLocation, FurthestSectionIndex.Z);
	 			}
	 			
	 			LandGeneratorThread* Thread = WorkingThreads[index];
	 			WorkingThreads.RemoveAt(index);
	 			FreeThread.Add(Thread);
	 		}
	 	}
		break;
	case false:
		
		if (LastWorkingThreadChecked < WorkingThreads.Num() - 1)
		{
			LastWorkingThreadChecked++;
		}
		else
		{
			LastWorkingThreadChecked = 0;
		}

		if (WorkingThreads.Num() > 0 && WorkingThreads[LastWorkingThreadChecked]->bInputReady == false)
		{
			InGenerationMap.Remove(WorkingThreads[LastWorkingThreadChecked]->SectionLocation);
			if (LastSectionIndex < MaxNumberOfSections)
			{
				GeneratedSection.Add(WorkingThreads[LastWorkingThreadChecked]->SectionLocation, LastSectionIndex);
				
				 LandMesh->CreateMeshSection(LastSectionIndex, WorkingThreads[LastWorkingThreadChecked]->returnVal.Vertices, FixedIndices
				 , WorkingThreads[LastWorkingThreadChecked]->returnVal.Normals, WorkingThreads[LastWorkingThreadChecked]->returnVal.UVs
				 , WorkingThreads[LastWorkingThreadChecked]->returnVal.Colors, WorkingThreads[LastWorkingThreadChecked]->returnVal.Tangents, true);
				LandMesh->SetMaterial(LastSectionIndex, LandMaterial);
				
				LastSectionIndex++;
			}
			else
			{
				//Player Location
				FVector Origin = UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation();
				
				FIntVector3 FurthestSectionIndex = GetFurthestSectionIndex(FVector2f(Origin.X, Origin.Y));

				GeneratedSection.Remove(FIntPoint(FurthestSectionIndex.X, FurthestSectionIndex.Y));

				 LandMesh->ClearMeshSection(FurthestSectionIndex.Z);
				 LandMesh->CreateMeshSection(FurthestSectionIndex.Z, WorkingThreads[LastWorkingThreadChecked]->returnVal.Vertices, FixedIndices
				 , WorkingThreads[LastWorkingThreadChecked]->returnVal.Normals, WorkingThreads[LastWorkingThreadChecked]->returnVal.UVs
				 , WorkingThreads[LastWorkingThreadChecked]->returnVal.Colors, WorkingThreads[LastWorkingThreadChecked]->returnVal.Tangents, true);
				
				GeneratedSection.Add(WorkingThreads[LastWorkingThreadChecked]->SectionLocation, FurthestSectionIndex.Z);
				
			}
			
			LandGeneratorThread* Thread = WorkingThreads[LastWorkingThreadChecked];
			WorkingThreads.RemoveAt(LastWorkingThreadChecked);
			FreeThread.Add(Thread);
		}
		
		break;
	}
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

	int FurtherDistance = 0;
	int Distance =0;
	
	int i = 0;
	for (auto Element : GeneratedSection)
	{
		i++;
		Distance = GetDistanceToSection(GetPlayerTile(), Element.Key);
		if ( Distance > FurtherDistance)
		{
			switch (bFastFindSectionToReplace)
			{
			case true:
				if (Distance > GetDistanceToSection(/*SectionCount*/RenderBounds/2, FIntPoint(0,0)))
				{
					FurthestLocation = Element.Key;
					return FIntVector(FurthestLocation.X, FurthestLocation.Y, GeneratedSection[FurthestLocation]);
				}
				
			case false:
				FurtherDistance = Distance;
				FurthestLocation = Element.Key;
				break;
			}

		}
	}
	
	return FIntVector(FurthestLocation.X, FurthestLocation.Y, GeneratedSection[FurthestLocation]);
}

TArray<FIntPoint> ALandGenerator::GetSectionsInRadius(FVector2f Location)
{
	FIntPoint CenterLocation = GetSectionByLocation(Location);
	TArray<FIntPoint> ToReturn;
	
	for (int x = 0; x < RenderBounds; ++x)
	{
		for (int y = 0; y < RenderBounds; ++y)
		{
			FIntPoint ChunckLocation = FIntPoint(CenterLocation.X + x - ChunckRenderDistance, CenterLocation.Y + y - ChunckRenderDistance);
			ToReturn.Add(ChunckLocation);
		}
	}
	
	return ToReturn;
}

bool ALandGenerator::IsTileOutOfChunkDistance(FIntPoint SectionLocation)
{
	if (SectionLocation.X > GetPlayerTile().X + (RenderBounds - 1)  - ChunckRenderDistance || SectionLocation.X < GetPlayerTile().X - (RenderBounds - 1) + ChunckRenderDistance)
	{
		return true;
	}
	else if (SectionLocation.Y > GetPlayerTile().Y + (RenderBounds - 1) - ChunckRenderDistance || SectionLocation.Y < GetPlayerTile().Y - (RenderBounds - 1) + ChunckRenderDistance)
	{
		return true;
	}
	return false;
}

FVector2f ALandGenerator::GetSectionCenterLocation(FIntPoint SectionLocation)
{
	return FVector2f(SectionLocation.X * SectionSize + SectionSize / 2 , SectionLocation.Y * SectionSize + SectionSize / 2);
}

FIntPoint ALandGenerator::GetSectionByLocation(FVector2f ActorLocation)
{
	return FIntPoint(FMath::FloorToInt(ActorLocation.X / SectionSize), FMath::FloorToInt(ActorLocation.Y / SectionSize));
}

int ALandGenerator::GetDistanceToSection(FIntPoint Section1, FIntPoint Section2)
{
	int distance = FVector2f::Distance(FVector2f(Section1.X, Section1.Y), FVector2f(Section2.X, Section2.Y));
	return distance;
}

FIntPoint ALandGenerator::GetPlayerTile()
{
	return GetSectionByLocation(UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation());
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
	
	if (SectionsToGenerate.Num() >= MaxNumberOfSections)
	{
		SectionsToGenerate.Pop();
	}
	SectionsToGenerate.Add(SectionLocation);
	
}

bool ALandGenerator::IsSectionInGeneration(FIntPoint SectionLocation)
{
	return InGenerationMap.Contains(SectionLocation);
}

bool ALandGenerator::CanGenerateSection(FIntPoint SectionLocation)
{
	return !InGenerationMap.Contains(SectionLocation) && !GeneratedSection.Contains(SectionLocation) && !SectionsToGenerate.Contains(SectionLocation);
}

void ALandGenerator::GenerateVegetationOnSection(FIntPoint SectionLocation , FVegetation Trees)
{
	if (Trees.Mesh == nullptr || Trees.Material == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No mesh for vegetation"));
		return; //return if no trees
	}
	else if (Trees.Material == nullptr)
	{
		FString Message = FString::Printf(TEXT("No mesh or material for vegetation "));
		Message.Append(Trees.Mesh->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
		return; //return if no material
	}
	
	LandMeshInstances->SetStaticMesh(Trees.Mesh);
	LandMeshInstances->SetMaterial(0, Trees.Material);
	
	TArray<FVector2f>Locations = UPoissonDiscSampling::SeededPoissonDiskSampling(GetWorld(), Trees.MinDistance, 20 , SectionSize/2 , SectionSize/2 , SectionLocation);
	
	for (auto HorizontalLocation : Locations)
	{
		FVector FinalLocation = FVector(HorizontalLocation.X, HorizontalLocation.Y, UProceduralLandGenSubsystem::GetGroundHeightPosition(GetWorld(), HorizontalLocation));
		
		LandMeshInstances->AddInstance(FTransform(FRotator(0,0,0), FinalLocation, FVector(1,1,1)));
		
	}

	
}
#pragma region Debug

void ALandGenerator::DrawDebugSection(FIntPoint SectionLocation, float LifeTime, FColor Color)
{
	FVector2f Center = GetSectionCenterLocation(SectionLocation);
	DrawDebugBox(GetWorld(), FVector(Center.X , Center.Y , 0) , FVector(SectionSize / 2, SectionSize / 2, 1000) , Color, true, LifeTime, -1, 5);
}

#pragma endregion Debug
