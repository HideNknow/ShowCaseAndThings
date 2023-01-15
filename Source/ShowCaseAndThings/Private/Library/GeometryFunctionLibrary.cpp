// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/GeometryFunctionLibrary.h"

void UGeometryFunctionLibrary::GetNormal(FVector P1, FVector P2, FVector P3, FVector& Normal)
{
	FVector U = P2 - P1;
	FVector V = P3 - P1;

	Normal.X = U.Y * V.Z - U.Z * V.Y;
	Normal.Y = U.Z * V.X - U.X * V.Z;
	Normal.Z = U.X * V.Y - U.Y * V.X;
}
