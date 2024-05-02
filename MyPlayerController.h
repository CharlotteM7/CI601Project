#pragma once

#include "CoreMinimal.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

UCLASS()
class TG_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    ANavMeshBoundsVolume* MyNavMeshBoundsVolume;

public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
};
