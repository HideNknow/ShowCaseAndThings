// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LandGenLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SHOWCASEANDTHINGS_API ULandGenLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable , Category = "LandGenLibrary")
	static FVector4 Test(FVector X);
	
};
