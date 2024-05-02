// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "KismetProceduralMeshLibrary.h"
#include "MyTerrainSaveGame.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h" 
#include "GameFramework/PlayerStart.h"
#include "WorldGenerator.generated.h"

USTRUCT(BlueprintType)
struct  FFoliageInstanceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Trees")
	TArray<int> Instances;
};





UCLASS()
class TG_API AWorldGenerator : public AActor
{

	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldGenerator();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	bool LoadingFromSave = false;

	ANavMeshBoundsVolume* NavMeshBoundsVolume;

	UPROPERTY( BlueprintReadWrite, Category = "World")
	TSubclassOf<ACharacter> PlayerCharacterClass;

	UPROPERTY( BlueprintReadWrite, Category = "Spawn")
	TEnumAsByte<EPhysicalSurface> SupportedSurfaceType;

	UPROPERTY( BlueprintReadWrite, Category = "Spawn")
	int PlayerSpawnHeightOffset = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "World")
	int MinHeightAboveTerrain = 250.0f;

	//**** Sea Variables ****//	

	UPROPERTY(BlueprintReadonly, Category = "Sea")
	float seaScale = 4000;

	UPROPERTY( BlueprintReadonly, Category = "Sea")
	float seaLevel = 0;

	UPROPERTY( BlueprintReadonly, Category = "Sea")
	bool enableSea = true;

	UPROPERTY(BlueprintReadonly, Category = "Sea")
	float Relocate = 10000.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sea")
	UStaticMeshComponent* seaMesh;

	UPROPERTY(BlueprintReadWrite, Category = "Sea")
	UMaterialInterface* seaMaterial = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Sea")
	TArray<AActor*> ToMoveActors;

	//**** Land Variables ****//	


	UPROPERTY(BlueprintReadWrite, Category = "Land")
	bool RandomiseTerrainLayout = false;

	UPROPERTY(BlueprintReadWrite, Category = "Land")
	bool CustomTerrainLayout = false;

	UPROPERTY(BlueprintReadOnly, Category = "Land")
	UProceduralMeshComponent* TerrainMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	UMaterialInterface* TerrainMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Land")
	bool GeneratorBusy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Land")
	bool TileReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	float MountainHeight = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	float LandHeight = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	FVector2D PBalance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	float MountainScale = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	float LandScale = 60000.f;

	UPROPERTY( BlueprintReadWrite, Category = "Land")
	float CustomLandScale;

	UPROPERTY( BlueprintReadWrite, Category = "Land")
	float CustomMountainScale;

	UPROPERTY( BlueprintReadWrite, Category = "Land")
	float CustomLandHeight;

	UPROPERTY( BlueprintReadWrite, Category = "Land")
	float CustomMountainHeight;

	UPROPERTY( BlueprintReadonly, Category = "Land")
	int XVertexCount = 20;

	UPROPERTY( BlueprintReadonly, Category = "Land")
	int YVertexCount = 20;

	UPROPERTY(BlueprintReadonly, Category = "Land")
	float CellSize = 2000;

	UPROPERTY( BlueprintReadonly, Category = "Land")
	int NumOfSectionsX = 2;

	UPROPERTY( BlueprintReadonly, Category = "Land")
	int NumOfSectionsY = 2;

	UPROPERTY(BlueprintReadWrite, Category = "Land")
	int MeshSectionIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Land")
	TMap<FIntPoint, FIntPoint> QueuedTiles;

	UPROPERTY(BlueprintReadWrite, Category = "Land")
	TMap<FIntPoint, FIntPoint> RemoveLODQueue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Land")
	float TileReplaceableDistance;

	

	//**** Trees Variables ****//	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trees")
	TArray< UFoliageType_InstancedStaticMesh*> FoliageTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trees")
	bool RandomiseFoliage = true;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	int InitialSeed = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Trees")
	FRandomStream RandomStream;

	UPROPERTY(BlueprintReadWrite, Category = "Trees")
	TArray<FVector> FoliagePoints;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	int GrowthProbabilityPercentage = 20;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	int MaxClusterSize = 7;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	int InstanceOffset = 1000;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	int InstanceOffsetVariation = 200;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	FVector InstanceScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	float ScaleVariationMultiplier = .2f;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	float GroundSlopeAngleMax = 45;

	UPROPERTY( BlueprintReadWrite, Category = "Trees")
	float GroundSlopeAngleMin = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Trees")
	TArray<UInstancedStaticMeshComponent*> FoliageComponents;

	UPROPERTY(BlueprintReadWrite, Category = "Trees")
	TMap<UInstancedStaticMeshComponent*, FFoliageInstanceData> ReplaceableFoliagePool;
	//--------------

	const float FlatRadius = 3000.0f;
	const float FlatHeight = 250.0f;
	const float TransitionWidth = 3000.0f;
	const float TransitionEndRadius = FlatRadius + TransitionWidth;
	


	int SectionIndexX = 0;
	int SectionIndexY = 0;
	int CellLODLevel = 1;

	private:
		bool bPlayerSpawned = false;


		TArray<int32> Triangles;
		TArray<FVector> SubVertices;
		TArray<FVector2D> SubUVs;
		TArray<int32> SubTriangles;
		TArray<FVector> SubNormals;
		TArray<FProcMeshTangent> SubTangents;
		TArray<AActor*> SpawnedHealthItems;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;



public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//**** Save/Load ****//	

	UFUNCTION(BlueprintCallable, Category = "Save")
	void LoadTerrainLayout();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveTerrainLayout();


	//********************//
	// Tiles//
	//********************//
	

	UFUNCTION(BlueprintCallable, Category = "Land")
	int DrawTile();

	UFUNCTION(BlueprintCallable, Category = "Land")
	void UpdateAndRemoveOutdatedLODs();
	
	UFUNCTION(BlueprintCallable, Category = "Land")
	int UpdateMeshSections();
	
	UFUNCTION(BlueprintCallable, Category = "Land")
	void ClearMeshData();

	UFUNCTION(BlueprintCallable, Category = "Land")
	FVector GetPlayerLocation();

	UFUNCTION(BlueprintCallable, Category = "Land")
	FVector2D GetTileLocation(FIntPoint TileCoordinate);

	UFUNCTION(BlueprintCallable, Category = "Land")
	FIntPoint GetClosestQueuedTile();

	UFUNCTION(BlueprintCallable, Category = "Land")
	int GetFurthestUpdateableTile();

	//********************//
	// Land//
	//********************//


	UFUNCTION(BlueprintCallable, Category = "Land")
	void GenerateTerrainLayout();

	UFUNCTION(BlueprintCallable, Category = "Land")
	void GenerateTerrain(const int InSectionIndexX, const int InSectionIndexY, const int LODFactor);

	UFUNCTION(BlueprintCallable, Category = "Land")
	void RebuildNavMesh();

	UFUNCTION(BlueprintCallable, Category = "Land")
	void GenerateTerrainAsync(const int InSectionIndexX, const int InSectionIndexY, const int LODLevel);

	UFUNCTION(BlueprintCallable, Category = "Land")
	float GetHeight(const FVector2D Location);

	UFUNCTION(BlueprintCallable, Category = "Land")
	float CalculateProceduralHeight(FVector2D Location);

	UFUNCTION(BlueprintCallable, Category = "Land")
	float PerlinNoiseExtended(const FVector2D Location, const float Scale, const float Amplitude, const FVector2D offset);

	//********************//
	//**** Tree ****//
	//********************//

	UFUNCTION(BlueprintImplementableEvent, Category = "Trees")
	bool RemoveFoliageTile(const int TileIndex);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void RemoveFoliageTileCpp(const int TileIndex);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void AddFoliageInstances(const FVector InLocation);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void AddRelevantFoliageInstances(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	bool CheckSlope(const FVector& FloorNormal, UFoliageType_InstancedStaticMesh* FoliageType) const;

	UFUNCTION(BlueprintCallable, Category = "Trees")
	FVector RandomiseScale(const FVector& BaseScale, FRandomStream& Stream);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	FVector RandomiseOffset(float Offset, float OffsetVariation, FRandomStream& Stream);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void GenerateFoliageTile(int32 TerrainMeshSectionIndex);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void SpawnFoliageCluster(UFoliageType_InstancedStaticMesh* FoliageType, UInstancedStaticMeshComponent* FoliageIsmComponent, const FVector ClusterLocation);

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void InitialiseFoliageTypes();

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void RefreshFoliage();

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void FoliageRandomisation();

	UFUNCTION(BlueprintCallable, Category = "Trees")
	void ProcessFoliageSeeds(UFoliageType_InstancedStaticMesh* FoliageType, UInstancedStaticMeshComponent* FoliageIsmComponent, const FVector& BaseLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Trees")
	void TrySpawnFoliageInstance(UFoliageType_InstancedStaticMesh* FoliageType, UInstancedStaticMeshComponent* FoliageIsmComponent, const FVector& InstanceLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Trees")
	bool IsSpawnLocationValid(const FHitResult& HitResults, UFoliageType_InstancedStaticMesh* FoliageType);
	
	UFUNCTION(BlueprintCallable, Category = "Trees")
	FTransform CreateFoliageTransform(UFoliageType_InstancedStaticMesh* FoliageType, const FVector& Location);
	
	UFUNCTION(BlueprintCallable, Category = "Trees")
	void TrySpawnFoliageAtLocation(UFoliageType_InstancedStaticMesh* FoliageType, const FVector& Location);
	
	
	//********************//
	// Spawn Logic //
	//********************//


	UFUNCTION(BlueprintCallable, Category = "World")
	void SpawnPlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "World")
	void SpawnNPC();

	UFUNCTION(BlueprintCallable, Category = "World")
	void SpawnHealthItemAtGoalLocation(const FVector& GoalLocation);

	UFUNCTION(BlueprintCallable, Category = "World")
	bool IsPathValid(FVector StartLocation, FVector GoalLocation);

	UFUNCTION(BlueprintCallable, Category = "World")
	bool IsLocationSuitable(const FHitResult& HitResult);


	//********************//
	// Goal setting //
	//********************//


	UFUNCTION(BlueprintCallable, Category = "World")
	void SetGoalLocation();

	UFUNCTION(BlueprintCallable, Category = "World")
	void SetGoalLocation2();

	UFUNCTION(BlueprintCallable, Category = "World")
	void SetGoalLocation3();

	UFUNCTION(BlueprintCallable, Category = "World")
	void SetGoalLocationsComplete();

	
	//********************//
	//**** Sea ****//
	//********************//


	UFUNCTION(BlueprintCallable, Category = "Sea")
	void RelocateSea();

	UFUNCTION(BlueprintCallable, Category = "Sea")
	void UpdateSeaParameters();

	UFUNCTION(BlueprintCallable, Category = "Sea")
	void ActorsToMove();

	

};


class FAsyncWorldGenerator : public FNonAbandonableTask
{
public:
	FAsyncWorldGenerator(AWorldGenerator* InWorldGenerator) : WorldGenerator(InWorldGenerator) {}
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncWorldGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}
	void DoWork();
private:
	AWorldGenerator* WorldGenerator;
};