#pragma once

#include "CoreMinimal.h"
#include "Spawner.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "GrassSpawner.generated.h"

/**
 *
 */
UCLASS()
class TG_API AGrassSpawner : public ASpawner
{
	GENERATED_BODY()
public:



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
	TArray<UInstancedStaticMeshComponent*> FoliageComponents;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
	FRandomStream RandomStream;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "SpawnSettings")
	float MinSpawnHeight = 250.0f;


public:
	void SpawnObject(const FHitResult Hit, const FVector ParentTileCenter) override;

	void RemoveTile(const FVector TileCenter) override;
};