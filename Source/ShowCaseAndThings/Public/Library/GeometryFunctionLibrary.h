// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeometryFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SHOWCASEANDTHINGS_API UGeometryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Normal")
		static void GetNormal(FVector P1, FVector P2, FVector P3, FVector& Normal);
};
