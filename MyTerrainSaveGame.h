// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MyTerrainSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class TG_API UMyTerrainSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Save")
    FVector2D PBalance;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Save")
    float MountainHeight;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Save")
    float LandHeight;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Save")
    float MountainScale;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Save")
    float LandScale;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Save")
    FVector PlayerPosition;
	
};
