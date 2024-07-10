// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PoissonDiscSampling.h"

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
            if (grid[i][j] != FVector2f::ZeroVector)
                if (FVector2f::Distance(grid[i][j], p) < radius)
                    return false;
    return true;
}

void UPoissonDiscSampling::InsertPoint(TArray<TArray<FVector2f>>& grid, float cellsize, FVector2f point)
{
    int xindex = FMath::FloorToInt(point.X / cellsize);
    int yindex = FMath::FloorToInt(point.Y / cellsize);
    grid[xindex][yindex] = point;
}

TArray<FVector2f> UPoissonDiscSampling::PoissonDiskSampling(float radius, int k, int width, int height)
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
        for (int tries = 0; tries < k; tries++) {
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
