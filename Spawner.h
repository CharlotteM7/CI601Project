

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "ProceduralMeshComponent.h"
#include "Spawner.generated.h"

UCLASS()
class TG_API ASpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASpawner();

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "SpawnGrid")
	int CellSize = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "SpawnGrid")
	int SubCellSize = 500;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "SpawnGrid")
	int SubCellRandomOffset = 200;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "SpawnGrid")
	int CellCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "SpawnGrid")
	int TraceDistance = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadwrite, Category = "SpawnGrid")
	int HeightMin = 300;

	TArray<FVector2D> SpawnedTiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
	TArray<UFoliageType_InstancedStaticMesh*> FoliageTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
	TEnumAsByte<EPhysicalSurface> SupportedSurfaceType;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit) const;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	FVector GetPlayerCell();

	UFUNCTION(BlueprintCallable)
	void UpdateTiles();

	UFUNCTION(BlueprintCallable)
	void UpdateTile(const FVector TileCenter);

	UFUNCTION(BlueprintCallable)
	virtual void SpawnObject(const FHitResult Hit, const FVector ParentTileCenter);

	UFUNCTION(BlueprintCallable)
	void RemoveFarTiles();

	UFUNCTION(BlueprintCallable)
	virtual void RemoveTile(const FVector TileCenter);

};