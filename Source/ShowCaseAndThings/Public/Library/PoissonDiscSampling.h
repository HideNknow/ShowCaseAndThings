// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoissonDiscSampling.generated.h"

/**
 * 
 */
UCLASS(meta=(BlueprintThreadSafe, ScriptName = "PoissonDiscSampling"))
class UPoissonDiscSampling : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	static bool IsValidPoint(TArray<TArray<FVector2f>>& grid, float cellsize ,int gwidth, int gheight, FVector2f p, float radius, int width, int height);

	static void InsertPoint(TArray<TArray<FVector2f>>& grid, float cellsize, FVector2f point);

	UFUNCTION(BlueprintCallable, Category = "PoissonDiscSampling")
	static TArray<FVector2f> PoissonDiskSampling(float radius, int k, int width, int height);
	
};
