// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldGenerator.h"
#include "GameFramework/GameModeBase.h"
#include "TGGameMode.generated.h"

UCLASS(minimalapi)
class ATGGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATGGameMode();

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game")
    TArray<FVector> GoalLocations;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game")
    FVector GoalLocation;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game")
    FVector GoalLocation2;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game")
    FVector GoalLocation3;
    


};
