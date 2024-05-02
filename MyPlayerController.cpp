// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "EngineUtils.h"



void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

   

    for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
    {
        MyNavMeshBoundsVolume = *It;
        break; 
    }
}

void AMyPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (MyNavMeshBoundsVolume && GetPawn())
    {
        FVector NewLocation = GetPawn()->GetActorLocation();

        // You might want to add some offset or adjustment to NewLocation based on your needs

        MyNavMeshBoundsVolume->SetActorLocation(NewLocation);

        // Optionally, you could resize the Nav Mesh Bounds Volume if needed
        // MyNavMeshBoundsVolume->SetBoxExtent(NewSize);
    }

  

}
