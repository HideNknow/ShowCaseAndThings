// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/LandGenerator.h"

#include "Engine/GameEngine.h"
#include "LandGenerator/LandGeneratorThread.h"



#include "Kismet/GameplayStatics.h"
#include "LandGenerator/CustomProceduralMeshComponent.h"
#include "LandGenerator/ProceduralLandGenSubsystem.h"
#include "LandGenerator/CustomProceduralMeshComponent.h"

// Sets default values
ALandGenerator::ALandGenerator()//Magic
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LandMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LandMesh"));
	LandMesh->SetupAttachment(this->GetRootComponent());
	LandMesh->bUseAsyncCooking = true;
	LandMesh->bUseComplexAsSimpleCollision = false;

	LandMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LandMeshInstances"));
	LandMeshInstances->SetupAttachment(this->GetRootComponent());
	
	NoiseGroundSettings.Seed = FVector2f(0.1f, 0.1f);
	NoiseGroundSettings.NoiseAmplitude = 1000.0f;
	NoiseGroundSettings.NoiseFrequency = 1.0f;
	
}

ALandGenerator::~ALandGenerator()
{}

// Called when the game starts or when spawned
void ALandGenerator::BeginPlay()
{

	LandMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LandMeshInstances->SetMobility(EComponentMobility::Type::Stationary);
	LandMeshInstances->SetCastShadow(false);
	LandMeshInstances->SetVisibility(true);
	
	PlayerSection = FIntPoint(-11111,-111111);

	//Bind
	UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnPlayerChangedSection.AddDynamic(this , &ALandGenerator::EventOnPlayerChangedSection);
	UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnSectionGenerated.AddDynamic(this , &ALandGenerator::EventOnSectionGenerated);
	NoiseGroundSettings.NoiseFrequency = NoiseGroundSettings.NoiseFrequency / SectionSize;
	UProceduralLandGenSubsystem::SetNoiseGroundSettings(GetWorld(), NoiseGroundSettings);;

	UProceduralLandGenSubsystem::AddNoiseGroundLayer(GetWorld(),NoiseGroundLayers);
	
	VertexSpacing = SectionSize /( SectionVertexCount -1);
	RenderBounds = ChunckRenderDistance*2+1;
	MaxNumberOfSections = RenderBounds * RenderBounds;

	GeneratedSectionsArray.Init(FIntPoint(-11111,-111111), MaxNumberOfSections);
	
	GenerateSectionIndices();
	
	for (int i = 0; i < MaxThreadNumber; ++i)
	{
		ThreadArray.Add(new LandGeneratorThread(this, FIntPoint(0, 0)));
	}
	FreeThread = ThreadArray;
	
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
	GEngine->AddOnScreenDebugMessage(-1, this->GetActorTickInterval(), FColor::Red, FString::Printf(TEXT("Sections generated: %f"), GetWorld()->DeltaRealTimeSeconds));

	SetActorTickInterval(GetWorld()->DeltaRealTimeSeconds*2);
	
	UpdatePlayerSection();
	
	UpdateThread();//Update the thread to check if the thread is done generating the section
	
	GenerateSectionAsync(); //Iterate over the sections to generate and check if we can assign them to workers
	
	Super::Tick(DeltaTime);
}

int ALandGenerator::AddSectionToArrays(FIntPoint SectionLocation)
{
	int index = FindIndexInArray(SectionLocation);
	GeneratedSectionsArray[index] = SectionLocation;
	return index;
}

int ALandGenerator::FindIndexInArray(FIntPoint SectionLocation ,bool PrintDebug)
{
	int ArraySize = RenderBounds * RenderBounds ;
	FIntPoint MaxIndexSize = FIntPoint(floor((ArraySize) / RenderBounds) , floor((ArraySize) / RenderBounds) * RenderBounds); //Max size of 1D array x and y
	
	int iX = SectionLocation.X % (RenderBounds);
	int iY = SectionLocation.Y % (RenderBounds);

	
	if (iX < 0)
	{
		iX = RenderBounds + iX;
	}
	if (iY < 0)
	{
		iY = RenderBounds + iY;
	}
	
	if (PrintDebug)
	{
		//print and location iX and iY and resutl
		GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds(), FColor::Red,
			FString::Printf(TEXT("Location: %d;%d      iXiY: %d;%d"), SectionLocation.X , SectionLocation.Y ,iX, iY));
	}

	int index = iX + iY * (RenderBounds);

	return abs(index);
}

bool ALandGenerator::IsSectionInArrays(FIntPoint SectionLocation)
{
	return GeneratedSectionsArray[FindIndexInArray(SectionLocation)] == SectionLocation;
}

FIntPoint ALandGenerator::GetIndexInArray(int Index)
{
	return 0;
}

void ALandGenerator::UpdatePlayerSection()
{
	FIntPoint NewPlayerSection = GetPlayerSection();
	if (NewPlayerSection != PlayerSection)
	{
		PlayerSection = NewPlayerSection;
		UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnPlayerChangedSection.Broadcast(PlayerSection);
	}
}

void ALandGenerator::EventOnPlayerChangedSection(FIntPoint NewSection)
{
	AddSectionToGenerate(GetSectionsInRenderBound(GetSectionCenterLocation(GetPlayerSection())));
	return;
}

void ALandGenerator::EventOnSectionGenerated(FIntPoint SectionLocation)
{
	for (auto Element : VegetationArray_TreeType)
	{
		GenerateTreeTypeVegetationOnSection(SectionLocation , Element);
	}

}

void ALandGenerator::GenerateSectionAsync()
{
	if (SectionsToGenerate.Num() <= 0 ||  FreeThread.Num() <= 0)
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
			if (InGenerationMap.Contains(SectionsToGenerate[indexToProcess]) || IsSectionInArrays(SectionsToGenerate[indexToProcess]))
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
	 			
	 			if (LandMesh->GetNumSections() -1  < MaxNumberOfSections) //Si on a pas atteint le nombre maximum de sections on ajoute la section
	 			{
	 				int ChangedIndex = AddSectionToArrays(WorkingThreads[index]->SectionLocation);
	 				
	 				LandMesh->CreateMeshSection(ChangedIndex, WorkingThreads[index]->returnVal.Vertices, FixedIndices
					 , WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
					 , WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents, true);
	 				LandMesh->SetMaterial(ChangedIndex, LandMaterial);

	 				UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnSectionGenerated.Broadcast(WorkingThreads[index]->SectionLocation);
	 			}
	 			else
	 			{

	 				int ChangedIndex = AddSectionToArrays(WorkingThreads[LastWorkingThreadChecked]->SectionLocation); //index de la section a changer
	 				
	 			    //LandMesh->ClearMeshSection(ChangedIndex);
	 			    // LandMesh->CreateMeshSection(ChangedIndex, WorkingThreads[index]->returnVal.Vertices, FixedIndices
	 			    // , WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
	 			    // , WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents, true);

	 				
	 				LandMesh->UpdateMeshSection(ChangedIndex, WorkingThreads[index]->returnVal.Vertices
					, WorkingThreads[index]->returnVal.Normals, WorkingThreads[index]->returnVal.UVs
					, WorkingThreads[index]->returnVal.Colors, WorkingThreads[index]->returnVal.Tangents);

	 				UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnSectionGenerated.Broadcast(WorkingThreads[index]->SectionLocation);
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
			if (LandMesh->GetNumSections() -1 < MaxNumberOfSections)
			{
				int ChangedIndex = AddSectionToArrays(WorkingThreads[LastWorkingThreadChecked]->SectionLocation);
				 LandMesh->CreateMeshSection(ChangedIndex, WorkingThreads[LastWorkingThreadChecked]->returnVal.Vertices, FixedIndices
				 , WorkingThreads[LastWorkingThreadChecked]->returnVal.Normals, WorkingThreads[LastWorkingThreadChecked]->returnVal.UVs
				 , WorkingThreads[LastWorkingThreadChecked]->returnVal.Colors, WorkingThreads[LastWorkingThreadChecked]->returnVal.Tangents, true);
				LandMesh->SetMaterial(ChangedIndex, LandMaterial);

				UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnSectionGenerated.Broadcast(WorkingThreads[LastWorkingThreadChecked]->SectionLocation);
				
			}
			else
			{

				int ChangedIndex = AddSectionToArrays(WorkingThreads[LastWorkingThreadChecked]->SectionLocation);
				
				 // LandMesh->ClearMeshSection(ChangedIndex);
				 // LandMesh->CreateMeshSection(ChangedIndex, WorkingThreads[LastWorkingThreadChecked]->returnVal.Vertices, FixedIndices
				 // , WorkingThreads[LastWorkingThreadChecked]->returnVal.Normals, WorkingThreads[LastWorkingThreadChecked]->returnVal.UVs
				 // , WorkingThreads[LastWorkingThreadChecked]->returnVal.Colors, WorkingThreads[LastWorkingThreadChecked]->returnVal.Tangents, true);
				LandMesh->UpdateMeshSection(ChangedIndex, WorkingThreads[LastWorkingThreadChecked]->returnVal.Vertices
				, WorkingThreads[LastWorkingThreadChecked]->returnVal.Normals, WorkingThreads[LastWorkingThreadChecked]->returnVal.UVs
				, WorkingThreads[LastWorkingThreadChecked]->returnVal.Colors, WorkingThreads[LastWorkingThreadChecked]->returnVal.Tangents);

				UProceduralLandGenSubsystem::GetSubsystem(GetWorld())->OnSectionGenerated.Broadcast(WorkingThreads[LastWorkingThreadChecked]->SectionLocation);
	
				
			}
			
			LandGeneratorThread* Thread = WorkingThreads[LastWorkingThreadChecked];
			WorkingThreads.RemoveAt(LastWorkingThreadChecked);
			FreeThread.Add(Thread);
		}
		
		break;
	}
}

TArray<FIntPoint> ALandGenerator::GetSectionsInRenderBound(FVector2f Location)
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
	if (SectionLocation.X > GetPlayerSection().X + (RenderBounds - 1)  - ChunckRenderDistance || SectionLocation.X < GetPlayerSection().X - (RenderBounds - 1) + ChunckRenderDistance)
	{
		return true;
	}
	else if (SectionLocation.Y > GetPlayerSection().Y + (RenderBounds - 1) - ChunckRenderDistance || SectionLocation.Y < GetPlayerSection().Y - (RenderBounds - 1) + ChunckRenderDistance)
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

FIntPoint ALandGenerator::GetPlayerSection()
{
	return GetSectionByLocation(UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation());
}

void ALandGenerator::AddSectionToGenerate(TArray<FIntPoint> SectionsLocation)
{
	SectionsToGenerate = SectionsLocation;
}

bool ALandGenerator::IsSectionInGeneration(FIntPoint SectionLocation)
{
	return InGenerationMap.Contains(SectionLocation);
}

bool ALandGenerator::CanGenerateSection(FIntPoint SectionLocation)
{
	return !InGenerationMap.Contains(SectionLocation) && !IsSectionInArrays(SectionLocation) && !SectionsToGenerate.Contains(SectionLocation);
}

void ALandGenerator::GenerateTreeTypeVegetationOnSection(FIntPoint SectionLocation , FVegetation Trees)
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

	if ( LandMeshInstances->GetStaticMesh() != Trees.Mesh)
	{
		LandMeshInstances->SetStaticMesh(Trees.Mesh);
		LandMeshInstances->SetMaterial(0, Trees.Material);
	}

	int NumInstances = (UProceduralLandGenSubsystem::fSeededRandInRange(GetWorld(),Trees.MinDensity,Trees.MaxDensity,SectionLocation)) * FMath::Pow((SectionSize / 1000),2.0f) ;

	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("NumInstances : %d"), NumInstances));
	
	for (int i = 0; i < NumInstances; ++i)
	{
		FVector Location = FVector(
			UProceduralLandGenSubsystem::fSeededRandInRange(GetWorld(), 0, SectionSize, FVector2f(SectionLocation.X + i, SectionLocation.Y)),
			UProceduralLandGenSubsystem::fSeededRandInRange(GetWorld(), 0, SectionSize, FVector2f(SectionLocation.X , SectionLocation.Y+ i)   ), 0);
		Location += FVector(SectionLocation.X * SectionSize, SectionLocation.Y * SectionSize, 0);
		Location.Z = UProceduralLandGenSubsystem::GetGroundHeightPosition(GetWorld(), FVector2f(Location.X, Location.Y));


		FVector Scale = FVector(UProceduralLandGenSubsystem::fSeededRandInRange(GetWorld(), Trees.ScaleVariationMin.X, Trees.ScaleVariationMax.X, FVector2f(SectionLocation.X, SectionLocation.Y)),
			UProceduralLandGenSubsystem::fSeededRandInRange(GetWorld(), Trees.ScaleVariationMin.Y, Trees.ScaleVariationMax.Y, FVector2f(SectionLocation.X, SectionLocation.Y)),
			UProceduralLandGenSubsystem::fSeededRandInRange(GetWorld(), Trees.ScaleVariationMin.Z, Trees.ScaleVariationMax.Z, FVector2f(SectionLocation.X, SectionLocation.Y)));

		LandMeshInstances->AddInstance(FTransform(FRotator(0, 0, 0), Location, Scale * Trees.ScaleFactor));
	}

	
}


#pragma region Debug

void ALandGenerator::DrawDebugSection(FIntPoint SectionLocation, float LifeTime, FColor Color)
{
	FVector2f Center = GetSectionCenterLocation(SectionLocation);
	DrawDebugBox(GetWorld(), FVector(Center.X , Center.Y , 0) , FVector(SectionSize / 2, SectionSize / 2, 1000) , Color, true, LifeTime, -1, 5);
}

#pragma endregion Debug
