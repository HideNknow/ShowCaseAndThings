// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PoissonDiscSampling.h"

#include "kismet/GameplayStatics.h"
#include "LandGenerator/FastNoiseLite.h"
#include "LandGenerator/ProceduralLandGenSubsystem.h"

bool UPoissonDiscSampling::IsValidPoint(TArray<TArray<FVector2f>>& grid, float cellsize, int gwidth, int gheight,
                                        FVector2f p, float radius, int width, int height)
{
    if (p.X < 0 || p.X >= width || p.Y < 0 || p.Y >= height)
        return false;

    int xindex = FMath::FloorToInt(p.X / cellsize);
    int yindex = FMath::FloorToInt(p.Y / cellsize);
    int i0 = FMath::Max(xindex - 1, 0);
    int i1 = FMath::Min(xindex + 1, gwidth - 1);
    int j0 = FMath::Max(yindex - 1, 0);
    int j1 = FMath::Min(yindex + 1, gheight - 1);

    for (int i = i0; i <= i1; i++)
        for (int j = j0; j <= j1; j++)
        {
            if (grid[i][j] != FVector2f(0,0))
            {
                if (FVector2f::Distance(grid[i][j], p) < radius)
                {
                    return false;
                }

            }
                
        }
          
              
    return true;
}

void UPoissonDiscSampling::InsertPoint(TArray<TArray<FVector2f>>& grid, float cellsize, FVector2f point)
{
    int xindex = FMath::FloorToInt(point.X / cellsize);
    int yindex = FMath::FloorToInt(point.Y / cellsize);
    grid[xindex][yindex] = point;
}

TArray<FVector2f> UPoissonDiscSampling::PoissonDiskSampling(float radius, int Tries, int width, int height)
{
    int N = 2;
    TArray<FVector2f> points;
    TArray<FVector2f> active;
    FVector2f p0(FMath::RandRange(0, width), FMath::RandRange(0, height));
    TArray<TArray<FVector2f>> grid;
    float cellsize = FMath::FloorToFloat(radius/sqrt(N));

    int ncells_width = FMath::CeilToInt(width/cellsize) + 1;
    int ncells_height = FMath::CeilToInt(height/cellsize) + 1;

    grid.SetNum(ncells_width);
    for (auto& Array : grid)
        Array.SetNum(ncells_height);

    InsertPoint(grid, cellsize, p0);
    points.Add(p0);
    active.Add(p0);

  
    
    while (active.Num() > 0) {
        int random_index = FMath::RandRange(0, active.Num() - 1);
        FVector2f p = active[random_index];

        bool found = false;
        for (int tries = 0; tries < Tries; tries++) {
            float theta = FMath::RandRange(0, 360);
            float new_radius = FMath::RandRange(radius, 2*radius);
            float pnewx = p.X + new_radius * FMath::Cos(FMath::DegreesToRadians(theta));
            float pnewy = p.Y + new_radius * FMath::Sin(FMath::DegreesToRadians(theta));
            FVector2f pnew(pnewx, pnewy);

            if (!IsValidPoint(grid, cellsize, ncells_width, ncells_height, pnew, radius, width, height))
                continue;

            points.Add(pnew);
            InsertPoint(grid, cellsize, pnew);
            active.Add(pnew);
            found = true;
            break;
        }

        if (!found)
            active.RemoveAt(random_index);
    }

    return points;
}

TArray<FVector2f> UPoissonDiscSampling::SeededPoissonDiskSampling( const UObject* WorldContextObject, float radius, int Tries, int width, int height , FVector2f SectionLocation)
{
    
    int N = 2;
    TArray<FVector2f> points;
    TArray<FVector2f> active;
    
    FVector2f p0(UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject, 0, width , SectionLocation)
        , UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject, 0, height , SectionLocation));
    
    TArray<TArray<FVector2f>> grid;
    const float Cellsize = trunc(FMath::FloorToFloat(radius/sqrt(N)));

    int ncells_width = FMath::CeilToInt(width/Cellsize) + 1;
    int ncells_height =FMath::CeilToInt(height/Cellsize) + 1;

    for (int i = 0; i < ncells_width; ++i)
    {
        grid.Add(TArray<FVector2f>());
        for (int j = 0; j < ncells_height; ++j)
        {
            grid[i].Add(FVector2f(0,0));
        }
    }

    InsertPoint(grid, Cellsize, p0);
    points.Add(p0);
    active.Add(p0);
    
    while (active.Num() > 0) {
        int random_index = UProceduralLandGenSubsystem::iSeededRandInRange(WorldContextObject
            , 0, active.Num() - 1 , SectionLocation + active.Num());
        
        
        FVector2f p = active[random_index];

        bool found = false;
        for (int tries = 0; tries < Tries; tries++) {
            
            float Theta = UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject,
                0, 360,SectionLocation + tries);

            const float New_Radius = UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject,
                radius,2 * radius, SectionLocation + tries);

            float pnewx = p.X + New_Radius * FMath::Cos(FMath::DegreesToRadians(Theta));
            float pnewy = p.Y + New_Radius * FMath::Sin(FMath::DegreesToRadians(Theta));
            FVector2f pnew(pnewx, pnewy);

            if (!IsValidPoint(grid, Cellsize, ncells_width, ncells_height, pnew, radius, width, height))
            {continue;}

            points.Add(pnew);
            InsertPoint(grid, Cellsize, pnew);
            active.Add(pnew);
            found = true;
            break;
        }

        if (!found)
            active.RemoveAt(random_index);
    }

    return points;
}

TArray<FVector2f> UPoissonDiscSampling::SeededPoissonPerlinDiskSampling(const UObject* WorldContextObject, float radius,
    int Tries, int width, int height, FVector2f SectionLocation)
{
        int N = 2;
    TArray<FVector2f> points;
    TArray<FVector2f> active;
    
    FVector2f p0(UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject, 0, width , SectionLocation)
        , UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject, 0, height , SectionLocation));
    
    TArray<TArray<FVector2f>> grid;
    const float Cellsize = trunc(FMath::FloorToFloat(radius/sqrt(N)));

    int ncells_width = FMath::CeilToInt(width/Cellsize) + 1;
    int ncells_height =FMath::CeilToInt(height/Cellsize) + 1;

    for (int i = 0; i < ncells_width; ++i)
    {
        grid.Add(TArray<FVector2f>());
        for (int j = 0; j < ncells_height; ++j)
        {
            grid[i].Add(FVector2f(0,0));
        }
    }

    InsertPoint(grid, Cellsize, p0);
    points.Add(p0);
    active.Add(p0);
    
    while (active.Num() > 0) {
        int random_index = UProceduralLandGenSubsystem::iSeededRandInRange(WorldContextObject
            , 0, active.Num() - 1 , SectionLocation + active.Num());
        
        
        FVector2f p = active[random_index];

        bool found = false;
        for (int tries = 0; tries < Tries; tries++) {
            
            float Theta = UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject,
                0, 360,SectionLocation + tries);

            const float New_Radius = UProceduralLandGenSubsystem::fSeededRandInRange(WorldContextObject,
                radius,2 * radius, SectionLocation + tries);

            float pnewx = p.X + New_Radius * FMath::Cos(FMath::DegreesToRadians(Theta));
            float pnewy = p.Y + New_Radius * FMath::Sin(FMath::DegreesToRadians(Theta));
            FVector2f pnew(pnewx, pnewy);

            if (!IsValidPoint(grid, Cellsize, ncells_width, ncells_height, pnew, radius, width, height))
            {continue;}

            points.Add(pnew);
            InsertPoint(grid, Cellsize, pnew);
            active.Add(pnew);
            found = true;
            break;
        }

        if (!found)
            active.RemoveAt(random_index);
    }

    return points;
}

