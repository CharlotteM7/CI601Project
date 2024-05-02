// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGenerator.h"
#include "KismetProceduralMeshLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Async/Async.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TGGameMode.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"


AWorldGenerator::AWorldGenerator()
{
	
	PrimaryActorTick.bCanEverTick = false;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	
	TerrainMesh->bUseAsyncCooking = true;
	TerrainMesh->SetupAttachment(GetRootComponent());
	

	TileReplaceableDistance = CellSize * (NumOfSectionsX + NumOfSectionsY) / 2 * (XVertexCount + YVertexCount);
	

	seaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeaMesh"));
	seaMesh->SetupAttachment(GetRootComponent()); 
	seaMesh->SetMaterial(0, seaMaterial);

}

void AWorldGenerator::BeginPlay()
{
	Super::BeginPlay();

	UpdateSeaParameters();
	FoliageRandomisation();
	InitialiseFoliageTypes();

	if (TerrainMesh)
	{
		TerrainMesh->RegisterComponentWithWorld(GetWorld());
		if (TerrainMesh->IsRegistered() && GetWorld())
		{
			TerrainMesh->SetCanEverAffectNavigation(true);
			FNavigationSystem::UpdateComponentData(*TerrainMesh);
		}
	}
}


void AWorldGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ActorsToMove();
	RelocateSea();

	
	RebuildNavMesh();
		
	
}

void AWorldGenerator::SaveTerrainLayout()
{
	UMyTerrainSaveGame* SaveGameInstance = 
		Cast<UMyTerrainSaveGame>(UGameplayStatics::CreateSaveGameObject(UMyTerrainSaveGame::StaticClass()));

	// Set save game properties from this class's members
	SaveGameInstance->PBalance = PBalance;
	SaveGameInstance->MountainHeight = MountainHeight;
	SaveGameInstance->LandHeight = LandHeight;
	SaveGameInstance->MountainScale = MountainScale;
	SaveGameInstance->LandScale = LandScale;


	// Save the game to a slot
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("TerrainLayoutSaveSlot"), 0);
}

void AWorldGenerator::LoadTerrainLayout()
{
	UMyTerrainSaveGame* LoadGameInstance = 
		Cast<UMyTerrainSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("TerrainLayoutSaveSlot"), 0));
	if (LoadGameInstance)
	{
		// Apply the loaded values to this class's members
		PBalance = LoadGameInstance->PBalance;
		MountainHeight = LoadGameInstance->MountainHeight;
		LandHeight = LoadGameInstance->LandHeight;
		MountainScale = LoadGameInstance->MountainScale;
		LandScale = LoadGameInstance->LandScale;
	}

	SpawnPlayerCharacter();


}


//********************//
// Tiles//
//********************//

void AWorldGenerator::UpdateAndRemoveOutdatedLODs() {
	TArray<FIntPoint> keyArray, valueArray;
	RemoveLODQueue.GenerateKeyArray(keyArray);
	RemoveLODQueue.GenerateValueArray(valueArray);

	FIntPoint currentSection(SectionIndexX, SectionIndexY);
	if (RemoveLODQueue.Contains(currentSection)) {
		FIntPoint* value = RemoveLODQueue.Find(currentSection);
		if (value) {
			RemoveFoliageTileCpp(value->X);
			TerrainMesh->ClearMeshSection(value->X);
			RemoveLODQueue.Remove(currentSection);
		}
	}
}

int AWorldGenerator::UpdateMeshSections() {
	int furthestTileIndex = GetFurthestUpdateableTile();
	if (furthestTileIndex > -1) {
		TArray<FIntPoint> keyArray, valueArray;
		QueuedTiles.GenerateKeyArray(keyArray);
		QueuedTiles.GenerateValueArray(valueArray);
		int replaceableMeshSection = valueArray[furthestTileIndex].X;
		FIntPoint replaceableTile = keyArray[furthestTileIndex];

		RemoveFoliageTileCpp(replaceableMeshSection);
		TerrainMesh->ClearMeshSection(replaceableMeshSection);
		TerrainMesh->CreateMeshSection(replaceableMeshSection, SubVertices, SubTriangles, SubNormals, SubUVs, TArray<FColor>(), SubTangents, true);
		QueuedTiles.Add(FIntPoint(SectionIndexX, SectionIndexY), FIntPoint(replaceableMeshSection, CellLODLevel));
		QueuedTiles.Remove(replaceableTile);

		return replaceableMeshSection;
	}
	else {
		TerrainMesh->CreateMeshSection(MeshSectionIndex, SubVertices, SubTriangles, SubNormals, SubUVs, TArray<FColor>(), SubTangents, true);
		if (TerrainMaterial) {
			TerrainMesh->SetMaterial(MeshSectionIndex, TerrainMaterial);
		}
		return MeshSectionIndex++;
	}
}

void AWorldGenerator::ClearMeshData() {
	SubVertices.Empty();
	SubNormals.Empty();
	SubUVs.Empty();
	SubTangents.Empty();
}

int AWorldGenerator::DrawTile() {
	// Update and check outdated LODs
	UpdateAndRemoveOutdatedLODs();

	int drawnMeshSection = UpdateMeshSections();

	// Clear temporary mesh data
	ClearMeshData();

	return drawnMeshSection;
}

void AWorldGenerator::GenerateFoliageTile(int32 TerrainMeshSectionIndex)
{
	if (TerrainMesh)
	{
		// Attempt to get a pointer to the procedural mesh section
		FProcMeshSection* MeshSection = TerrainMesh->GetProcMeshSection(TerrainMeshSectionIndex);

		for (const FProcMeshVertex& Vertex : MeshSection->ProcVertexBuffer)
		{

			FVector ActorLocation = GetActorLocation();

			FVector LocationToAddFoliage = ActorLocation + Vertex.Position;

			AddFoliageInstances(LocationToAddFoliage);
			AddRelevantFoliageInstances(LocationToAddFoliage);
		}

		RefreshFoliage();

	}
}

FVector AWorldGenerator::GetPlayerLocation()
{



	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (PlayerCharacter)
	{
		return PlayerCharacter->GetActorLocation();
	}
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (PlayerPawn)
	{
		return PlayerPawn->GetActorLocation();
	}

	return FVector(0, 0, 0);
}

FVector2D AWorldGenerator::GetTileLocation(FIntPoint TileCoordinate)
{
	return FVector2D(TileCoordinate * FIntPoint(XVertexCount - 1, YVertexCount - 1) * CellSize) + FVector2D(XVertexCount - 1, YVertexCount - 1) * CellSize / 2;
}

FIntPoint AWorldGenerator::GetClosestQueuedTile()
{
	float ClosestDistance = TNumericLimits<float>::Max();
	FIntPoint ClosestTile;
	for (const auto& Entry : QueuedTiles)
	{
		const FIntPoint& Key = Entry.Key;
		int Value = Entry.Value.X;
		if (Value == -1)
		{
			FVector2D TileLocation = GetTileLocation(Key);
			FVector PlayerLocation = GetPlayerLocation();
			float Distance = FVector2D::Distance(TileLocation, FVector2D(PlayerLocation));
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestTile = Key;
			}
		}
	}
	return ClosestTile;
}

int AWorldGenerator::GetFurthestUpdateableTile()
{
	float FurthestDistance = std::numeric_limits<float>::lowest();
	int FurthestTileIndex = -1;
	int CurrentIndex = 0;
	for (const auto& Entry : QueuedTiles)
	{
		const FIntPoint& Key = Entry.Key;
		int Value = Entry.Value.X;
		if (Value != -1)
		{
			FVector2D TileLocation = GetTileLocation(Key);
			FVector PlayerLocation = GetPlayerLocation();
			float Distance = FVector2D::Distance(TileLocation, FVector2D(PlayerLocation));
			if (Distance > FurthestDistance && Distance > TileReplaceableDistance)
			{
				FurthestDistance = Distance;
				FurthestTileIndex = CurrentIndex;
			}
		}
		CurrentIndex++;
	}
	return FurthestTileIndex;
}

void AWorldGenerator::RemoveFoliageTileCpp(const int TileIndex)
{
	TArray<FProcMeshVertex> Vertices1 = TerrainMesh->GetProcMeshSection(TileIndex)->ProcVertexBuffer;
	FVector FirstVertex = Vertices1[0].Position;
	FVector LastVertex = Vertices1[Vertices1.Num() - 1].Position;
	FBox Box = FBox(FVector(FirstVertex.X, FirstVertex.Y, (MountainHeight + LandHeight) * (-1)), FVector(LastVertex.X, LastVertex.Y, (MountainHeight + LandHeight)));

	for (int FoliageComponentIndex = 0; FoliageComponentIndex < FoliageComponents.Num(); FoliageComponentIndex++)
	{
		UInstancedStaticMeshComponent* FoliageComponent = FoliageComponents[FoliageComponentIndex];
		TArray<int> Instances = FoliageComponent->GetInstancesOverlappingBox(Box);

		FFoliageInstanceData* FolData = ReplaceableFoliagePool.Find(FoliageComponent);

		if (FolData)
		{
			FolData->Instances.Append(Instances);
		}
		else
		{
			ReplaceableFoliagePool.Add(FoliageComponent, FFoliageInstanceData(Instances));
		}
	}
}

//********************//
// Land//
//********************//

void AWorldGenerator::GenerateTerrainLayout()
{
	// Check if we are loading from a saved game
	if (LoadingFromSave)
	{
		LoadTerrainLayout();


	}
	else if (CustomTerrainLayout)
	{
		// User enters values 

		PBalance;
		MountainHeight = CustomMountainHeight;
		LandHeight = CustomLandHeight;
		MountainScale = CustomMountainScale;
		LandScale = CustomLandScale;


	}
	else if (RandomiseTerrainLayout)
	{
		PBalance = FVector2D(FMath::FRandRange(0.f, 1000000.f), FMath::FRandRange(0.f, 1000000.f));
		MountainHeight = MountainHeight * FMath::FRandRange(.4f, 1.5f);
		LandHeight = LandHeight * FMath::FRandRange(.4f, 1.5f);
		MountainScale = MountainScale * FMath::FRandRange(.6f, 2.f);
		LandScale = LandScale * FMath::FRandRange(.8f, 2.f);


	}
	else {

		PBalance;
		MountainHeight;
		LandHeight;
		MountainScale;
		LandScale;

	}



}

void AWorldGenerator::GenerateTerrainAsync(const int InSectionIndexX,const int InSectionIndexY, const int LODLevel)
{
	GeneratorBusy = true;
	SectionIndexX = InSectionIndexX;
	SectionIndexY = InSectionIndexY;
	CellLODLevel = FMath::Max(1, LODLevel);

	QueuedTiles.Add(FIntPoint(InSectionIndexX, InSectionIndexY),
		FIntPoint(MeshSectionIndex, CellLODLevel));

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [&]()
		{
			auto WorldGenTask = new FAsyncTask<FAsyncWorldGenerator>(this);
			WorldGenTask->StartBackgroundTask();
			WorldGenTask->EnsureCompletion();
			delete WorldGenTask;
		}
	);

}

float AWorldGenerator::GetHeight(FVector2D Location)
{
	float DistFromCenter = FVector2D::Distance(Location, FVector2D(0, 0));

	if (DistFromCenter <= FlatRadius)
	{
		return FlatHeight; // Inside the central flat area
	}

	float ProceduralHeight = CalculateProceduralHeight(Location);

	if (DistFromCenter <= TransitionEndRadius)
	{
		// Transition zone
		float TransitionFactor = (DistFromCenter - FlatRadius) / TransitionWidth;
		return FMath::Lerp(FlatHeight, ProceduralHeight, TransitionFactor);
	}

	return ProceduralHeight; // Outside transition zone
}

float AWorldGenerator::CalculateProceduralHeight(FVector2D Location)
{
	return PerlinNoiseExtended(Location, 1 / MountainScale, MountainHeight, FVector2D(.1f)) +
		PerlinNoiseExtended(Location, 1 / LandScale, LandHeight, FVector2D(.2f)) +
		PerlinNoiseExtended(Location, .001f, 500, FVector2D(.3f)) +
		PerlinNoiseExtended(Location, .01f, 100, FVector2D(.4f));
}

float AWorldGenerator::PerlinNoiseExtended(const FVector2D Location, const float Scale, const float Amplitude, const FVector2D offset)
{
	FVector2D ScaledLocation = (Location * Scale) + offset + PBalance + FVector2D(.1f, .1f);
	return FMath::PerlinNoise2D(ScaledLocation) * Amplitude;
}

void FAsyncWorldGenerator::DoWork()
{
	
	
	WorldGenerator->GenerateTerrain(WorldGenerator->SectionIndexX, WorldGenerator->SectionIndexY, WorldGenerator->CellLODLevel);
}

void AWorldGenerator::RebuildNavMesh()
{
	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		NavSys->Build();
	}

}

void AWorldGenerator::GenerateTerrain(const int InSectionIndexX, const int InSectionIndexY, const int LODFactor)
{
	int LODXVertexCount = XVertexCount / LODFactor;
	int LODYVertexCount = YVertexCount / LODFactor;
	float LODCellSize = CellSize * LODFactor;

	float XGap = (XVertexCount * CellSize - LODXVertexCount * LODCellSize) / LODCellSize;
	float YGap = (YVertexCount * CellSize - LODYVertexCount * LODCellSize) / LODCellSize;

	LODXVertexCount += FMath::CeilToInt(XGap) + 1;
	LODYVertexCount += FMath::CeilToInt(YGap) + 1;


	FVector Offset = FVector(InSectionIndexX * (XVertexCount - 1), InSectionIndexY * (YVertexCount - 1), 0.f) * CellSize;

	TArray<FVector> Vertices;


	TArray<FVector2D> UVs;
	FVector2D UV;

	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;

	//Vertices and UVs
	for (int32 iVY = -1; iVY <= LODYVertexCount; iVY++)
	{
		for (int32 iVX = -1; iVX <= LODXVertexCount; iVX++)
		{
			// Vertex calculation
			FVector2D CurrentLocation(iVX * LODCellSize + Offset.X, iVY * LODCellSize + Offset.Y);
			float Z = GetHeight(CurrentLocation);
			FVector Vertex(CurrentLocation.X, CurrentLocation.Y, Z);
			Vertices.Add(Vertex);

			//UV
			UV.X = (iVX + (InSectionIndexX * (LODXVertexCount - 1))) * LODCellSize / 100;
			UV.Y = (iVY + (InSectionIndexY * (LODYVertexCount - 1))) * LODCellSize / 100;
			UVs.Add(UV);
		}
	}

	// Triangles

	Triangles.Empty();
	for (int32 iTY = 0; iTY <= LODYVertexCount; iTY++)
	{
		for (int32 iTX = 0; iTX <= LODXVertexCount; iTX++)
		{
			Triangles.Add(iTX + iTY * (LODXVertexCount + 2));
			Triangles.Add(iTX + (iTY + 1) * (LODXVertexCount + 2));
			Triangles.Add(iTX + iTY * (LODXVertexCount + 2) + 1);

			Triangles.Add(iTX + (iTY + 1) * (LODXVertexCount + 2));
			Triangles.Add(iTX + (iTY + 1) * (LODXVertexCount + 2) + 1);
			Triangles.Add(iTX + iTY * (LODXVertexCount + 2) + 1);
		}

	}
	//}

	// Calculating subset mesh to prevent seams

	int VertexIndex = 0;

	//calculate normals
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);

	// Subset vertices and UVs
	for (int32 iVY = -1; iVY <= LODYVertexCount; iVY++)
	{
		for (int32 iVX = -1; iVX <= LODXVertexCount; iVX++)
		{
			if (-1 < iVY && iVY < LODYVertexCount && -1 < iVX && iVX < LODXVertexCount)
			{
				SubVertices.Add(Vertices[VertexIndex]);
				SubUVs.Add(UVs[VertexIndex]);
				SubNormals.Add(Normals[VertexIndex]);
				SubTangents.Add(Tangents[VertexIndex]);
			}
			VertexIndex++;
		}
	}

	// Subset triangles

	SubTriangles.Empty();
	for (int32 iTY = 0; iTY <= LODYVertexCount - 2; iTY++)
	{
		for (int32 iTX = 0; iTX <= LODXVertexCount - 2; iTX++)
		{
			SubTriangles.Add(iTX + iTY * LODXVertexCount);
			SubTriangles.Add(iTX + iTY * LODXVertexCount + LODXVertexCount);
			SubTriangles.Add(iTX + iTY * LODXVertexCount + 1);

			SubTriangles.Add(iTX + iTY * LODXVertexCount + LODXVertexCount);
			SubTriangles.Add(iTX + iTY * LODXVertexCount + LODXVertexCount + 1);
			SubTriangles.Add(iTX + iTY * LODXVertexCount + 1);
		}
	}

	TileReady = true;


}


//********************//
// Trees//
//********************//

void AWorldGenerator::RefreshFoliage()
{
	for (UInstancedStaticMeshComponent* FoliageComponent : FoliageComponents)
	{
		if (FoliageComponent)
		{
			// Loop over each instance in the component
			const int32 InstanceCount = FoliageComponent->GetInstanceCount();
			for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex)
			{
				FTransform InstanceTransform;
				FoliageComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, false);


				// Update the instance with the new transform
				FoliageComponent->UpdateInstanceTransform(InstanceIndex, InstanceTransform, true, true, false);
			}

			// Mark the component as dirty to ensure it updates
			FoliageComponent->MarkRenderStateDirty();
		}
	}
}

FVector AWorldGenerator::RandomiseOffset(float OffsetR, float OffsetVariation, FRandomStream& Stream)
{
	// Randomize a float between -OffsetVariation and OffsetVariation
	float RandomX = Stream.FRandRange(-OffsetVariation, OffsetVariation);
	float RandomY = Stream.FRandRange(-OffsetVariation, OffsetVariation);
	float RandomZ = Stream.FRandRange(-OffsetVariation, OffsetVariation);

	// Combine the randomized offset with the base offset
	return FVector(OffsetR + RandomX, OffsetR + RandomY, OffsetR + RandomZ);
}

FVector AWorldGenerator::RandomiseScale(const FVector& BaseScale, FRandomStream& Stream)
{
	// Randomise scale uniformly in all directions
	float RandomScaleFactor = Stream.FRandRange(1.0f - ScaleVariationMultiplier, 1.0f + ScaleVariationMultiplier);
	return BaseScale * RandomScaleFactor;
}

bool AWorldGenerator::CheckSlope(const FVector& FloorNormal, UFoliageType_InstancedStaticMesh* FoliageType) const
{
	if (!FoliageType)
	{
		return false;
	}

	// Calculate the degree angle between the floor normal and the world up vector
	float SlopeDegreeAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(FloorNormal, FVector::UpVector)));

	// Check if the slope is within the range defined by the active foliage type
	return SlopeDegreeAngle >= GroundSlopeAngleMin && SlopeDegreeAngle <= GroundSlopeAngleMax;
}

void AWorldGenerator::InitialiseFoliageTypes()
{


	for (UFoliageType_InstancedStaticMesh* FoliageType : FoliageTypes)
	{
		// Only proceed if we have a valid foliage type
		if (FoliageType && FoliageType->GetStaticMesh())
		{
			// Create a new instanced static mesh component
			UInstancedStaticMeshComponent* ISMComp = NewObject<UInstancedStaticMeshComponent>(this);
			ISMComp->RegisterComponent();
			ISMComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			ISMComp->SetStaticMesh(FoliageType->GetStaticMesh());

			// Add the component to our foliage components array
			FoliageComponents.Add(ISMComp);
		}
	}
}

void AWorldGenerator::FoliageRandomisation()
{
	if (RandomiseFoliage)
	{
		// If we want to randomize the foliage, set the initial seed and prepare the random stream
		InitialSeed = FMath::RandRange(0, 1000);
		RandomStream.Initialize(InitialSeed);
	}
}

void AWorldGenerator::AddFoliageInstances(FVector InLocation)
{
	for (int FoliageTypeIndex = 0; FoliageTypeIndex < FoliageTypes.Num(); FoliageTypeIndex++)
	{
		UFoliageType_InstancedStaticMesh* FoliageType = FoliageTypes[FoliageTypeIndex];

		// Skip invalid foliage types 
		if (!FoliageType)
			continue;

		// Check foliage growing altitude
		if (InLocation.Z < FoliageType->Height.Min || InLocation.Z > FoliageType->Height.Max)
			continue;

		// Growth density check 
		if (FoliageType->InitialSeedDensity < RandomStream.FRandRange(0.f, 10.f))
			continue;

		UInstancedStaticMeshComponent* FoliageIsmComponent = FoliageComponents[FoliageTypeIndex];

		SpawnFoliageCluster(FoliageType, FoliageIsmComponent, InLocation);

	}

}

void AWorldGenerator::SpawnFoliageCluster(UFoliageType_InstancedStaticMesh* FoliageType, UInstancedStaticMeshComponent* FoliageIsmComponent, const FVector ClusterLocation) {
	int MaxSteps = RandomStream.RandRange(0, FoliageType->NumSteps);
	FVector ClusterBase = ClusterLocation;

	for (int Step = 0; Step < MaxSteps; Step++) {
		ClusterBase += RandomStream.GetUnitVector() * FoliageType->AverageSpreadDistance;
		ProcessFoliageSeeds(FoliageType, FoliageIsmComponent, ClusterBase);
	}
}

void AWorldGenerator::ProcessFoliageSeeds(UFoliageType_InstancedStaticMesh* FoliageType, UInstancedStaticMeshComponent* FoliageIsmComponent, const FVector& BaseLocation) {
	int MaxSeeds = RandomStream.RandRange(0, FoliageType->SeedsPerStep);
	for (int SeedIndex = 0; SeedIndex < MaxSeeds; SeedIndex++) {
		FVector InstanceLocation = BaseLocation + RandomStream.GetUnitVector() * FoliageType->SpreadVariance;
		TrySpawnFoliageInstance(FoliageType, FoliageIsmComponent, InstanceLocation);
	}
}

void AWorldGenerator::TrySpawnFoliageInstance(UFoliageType_InstancedStaticMesh* FoliageType, UInstancedStaticMeshComponent* FoliageIsmComponent, const FVector& InstanceLocation) {
	FHitResult HitResults;
	FCollisionQueryParams CollisionParams;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResults,
		InstanceLocation + FVector(0, 0, 2000),
		InstanceLocation + FVector(0, 0, -2000),
		ECC_Visibility,
		CollisionParams
	);

	if (bHit && IsSpawnLocationValid(HitResults, FoliageType)) {
		FTransform InstanceTransform = CreateFoliageTransform(FoliageType, HitResults.Location);
		FoliageIsmComponent->AddInstance(InstanceTransform);
	}
}

bool AWorldGenerator::IsSpawnLocationValid(const FHitResult& HitResults, UFoliageType_InstancedStaticMesh* FoliageType) {
	if (HitResults.Component != TerrainMesh) return false;

	float DotProduct = FVector::DotProduct(HitResults.ImpactNormal, FVector::UpVector);
	float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
	return SlopeAngle >= FoliageType->GroundSlopeAngle.Min && SlopeAngle <= FoliageType->GroundSlopeAngle.Max;
}

FTransform AWorldGenerator::CreateFoliageTransform(UFoliageType_InstancedStaticMesh* FoliageType, const FVector& Location) {
	FVector AdjustedLocation = Location + FVector(0, 0, RandomStream.FRandRange(FoliageType->ZOffset.Min, FoliageType->ZOffset.Max));
	FVector Scale = FVector::One() * RandomStream.FRandRange(FoliageType->ProceduralScale.Min, FoliageType->ProceduralScale.Max);
	FRotator Rotation = FoliageType->RandomYaw ? FRotator(0, RandomStream.FRandRange(0, 360), 0) : FRotator::ZeroRotator;

	return FTransform(Rotation, AdjustedLocation, Scale);
}

void AWorldGenerator::TrySpawnFoliageAtLocation(UFoliageType_InstancedStaticMesh* FoliageType, const FVector& Location) {
	// Randomise the offset for each instance within the provided range
	FVector Offset = RandomiseOffset(InstanceOffset, InstanceOffsetVariation, RandomStream);
	FVector Start = Location + Offset + FVector(0.f, 0.f, 20000.f);  // Start high above to ensure the line trace hits the landscape
	FVector End = Start - FVector(0.f, 0.f, 40000.f);  // End far below the start to cover all possible terrain elevations

	// Line trace to find where the foliage should be placed
	FHitResult HitResult;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility)) {
		if (HitResult.Component.Get() == TerrainMesh && CheckSlope(HitResult.ImpactNormal, FoliageType)) {
			FVector LocationWithOffset = HitResult.Location + FVector(0, 0, RandomStream.FRandRange(FoliageType->ZOffset.Min, FoliageType->ZOffset.Max));
			FTransform InstanceTransform = FTransform(FRotator::ZeroRotator, LocationWithOffset, FVector::One() * RandomStream.FRandRange(FoliageType->ProceduralScale.Min, FoliageType->ProceduralScale.Max));
			UInstancedStaticMeshComponent* FoliageIsmComponent = FoliageComponents[FoliageTypes.IndexOfByKey(FoliageType)];  // Ensure you have a way to reference the correct foliage component
			FoliageIsmComponent->AddInstance(InstanceTransform);
		}
	}
}

void AWorldGenerator::AddRelevantFoliageInstances(FVector Location) {
	for (UFoliageType_InstancedStaticMesh* FoliageType : FoliageTypes) {
		if (!FoliageType) continue;
		if (Location.Z < FoliageType->Height.Min || Location.Z > FoliageType->Height.Max) continue;
		if (RandomStream.FRandRange(0.f, 100.f) < GrowthProbabilityPercentage) {
			TrySpawnFoliageAtLocation(FoliageType, Location);
		}
	}
}




//********************//
// Spawn Logic //
//********************//

void AWorldGenerator::SpawnPlayerCharacter()
{
	if (bPlayerSpawned) return;

	// Ensure PlayerCharacterClass is valid
	if (!PlayerCharacterClass) {
		//UE_LOG(LogTemp, Warning, TEXT("PlayerCharacterClass is not set."));
		return;
	}

	FVector Start, End;
	bool bSuitableLocationFound = false;
	FVector FallbackSpawnPoint = FVector(0, 0, 550); // Fallback spawn location
	FVector SpawnPoint; // This will be determined dynamically or set to the fallback location

	FVector AreaCenter = FVector(0, 0, 0);
	float ScanRange = 80000;
	int NumTries = 100;



	for (int i = 0; i < NumTries; i++)
	{
		FVector RandomPoint = AreaCenter + FMath::RandPointInBox(FBox(FVector(-ScanRange, -ScanRange, 0),
			FVector(ScanRange, ScanRange, 0)));

		Start = FVector(RandomPoint.X, RandomPoint.Y, 1500);
		End = FVector(RandomPoint.X, RandomPoint.Y, 500);

		FHitResult HitResult;
		seaMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		FCollisionQueryParams TraceParams(FName(TEXT("PlayerSpawnTrace")), true);
		TraceParams.AddIgnoredComponent(seaMesh);
		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams);

		if (bHit && IsLocationSuitable(HitResult))
		{
			SpawnPoint = HitResult.Location + FVector(0, 0, PlayerSpawnHeightOffset);
			bSuitableLocationFound = true;
			break;
		}
	}

	if (!bSuitableLocationFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("No suitable spawn location found, using fallback location."));
		SpawnPoint = FallbackSpawnPoint;
	}

	// Spawn the player character at the determined SpawnPoint
	ACharacter* SpawnedCharacter = GetWorld()->SpawnActor<ACharacter>(PlayerCharacterClass,
		FTransform(FRotator::ZeroRotator, SpawnPoint));
	if (SpawnedCharacter)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
			PlayerController->Possess(SpawnedCharacter);
			bPlayerSpawned = true;
			//UE_LOG(LogTemp, Log, TEXT("Player character spawned and possessed at %s"), *SpawnPoint.ToString());

			SetGoalLocationsComplete();
			SpawnNPC();


		}

	}
}


void AWorldGenerator::SpawnNPC()
{
	// Ensure there is a valid player character to reference
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerCharacter) return;

	// Dynamically load the BP_Fox class
	UClass* FoxBlueprintClass =
		StaticLoadClass(ACharacter::StaticClass(), nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_Fox.BP_Fox_C"));
	if (!FoxBlueprintClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find the BP_Fox class."));
		return;
	}

	// Calculate spawn location directly in front of the player
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	FVector PlayerForwardVector = PlayerCharacter->GetActorForwardVector();
	FVector PlayerRightVector = PlayerCharacter->GetActorRightVector();


	const float DistanceInFront = 600.0f;
	const float DistanceToRight = 150.0f;

	FVector SpawnLocation = PlayerLocation + (PlayerForwardVector * DistanceInFront) +
		(PlayerRightVector * DistanceToRight);

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	// Perform the line trace to find the terrain mesh and suitable ground location
	FVector TraceStart = SpawnLocation + FVector(0.0f, 0.0f, 600.0f);
	FVector TraceEnd = SpawnLocation - FVector(0.0f, 0.0f, 600.0f);
	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(PlayerCharacter);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
	{
		if (HitResult.Component.IsValid() && HitResult.Component->IsA<UProceduralMeshComponent>())
		{
			// If we hit the ground, adjust the spawn location to be on the ground
			const float FoxBaseOffset = 100.0f; // Adjust this value as needed
			SpawnLocation.Z = HitResult.Location.Z + FoxBaseOffset; // Adjust spawn Z to be at the base of the fox mesh

			// Align the fox to the slope of the ground if necessary
			FVector Normal = HitResult.ImpactNormal;
			FRotator GroundRotation = FRotationMatrix::MakeFromZ(Normal).Rotator();

			//// Calculate the rotation to face the player
			FVector DirectionToPlayer = (PlayerLocation - SpawnLocation).GetSafeNormal();
			FRotator SpawnRotation = FRotationMatrix::MakeFromX(DirectionToPlayer).Rotator();

			//// Use the rotation facing the player
			FRotator FinalRotation = FRotator(GroundRotation.Pitch, SpawnRotation.Yaw, GroundRotation.Roll);

			// Spawn the BP_Fox
			AActor* SpawnedFox = GetWorld()->SpawnActor<AActor>(FoxBlueprintClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (!SpawnedFox)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to spawn BP_Fox."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit component is not the procedural mesh."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find the ground for the fox to spawn on."));
	}
}


void AWorldGenerator::SpawnHealthItemAtGoalLocation(const FVector& GoalLocation)
{
	FVector Start = GoalLocation + FVector(0, 0, 500); // Start trace above the terrain
	FVector End = GoalLocation - FVector(0, 0, 500); // End trace below the terrain

	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.bReturnPhysicalMaterial = true; 
	

	// Perform the trace
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams))
	{
		if (HitResult.Component.IsValid() && HitResult.Component->IsA<UProceduralMeshComponent>())
		{


			// Adjust the Z height of the health item to sit exactly on the terrain surface
			FVector HealthItemLocation = HitResult.Location;
			HealthItemLocation.Z += 100;

			// Spawn the health item
			UClass* HealthItemClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_Health.BP_Health_C"));
			if (HealthItemClass)
			{
				AActor* SpawnedHealthItem = GetWorld()->SpawnActor<AActor>(HealthItemClass, HealthItemLocation, FRotator::ZeroRotator);
			

			}

		}
	}

}

bool AWorldGenerator::IsPathValid(FVector StartLocation, FVector GoalLocation)
{
	bool bPathIsValid = false;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation NavStartLocation;
	FNavLocation NavGoalLocation;

	// Convert to nav locations
	NavSys->ProjectPointToNavigation(StartLocation, NavStartLocation);
	NavSys->ProjectPointToNavigation(GoalLocation, NavGoalLocation);

	// Generate the path
	FPathFindingQuery Query;
	FNavAgentProperties NavAgentProperties;
	Query = FPathFindingQuery(nullptr, *NavSys->GetDefaultNavDataInstance(),
		NavStartLocation.Location, NavGoalLocation.Location);
	FPathFindingResult Result = NavSys->FindPathSync(NavAgentProperties, Query);

	// Check if the path is valid
	if (Result.IsSuccessful() && Result.Path.IsValid())
	{
		bPathIsValid = true;
	}

	return bPathIsValid;

}

bool AWorldGenerator::IsLocationSuitable(const FHitResult& HitResult)
{
	// Verify that the hit component is the terrain mesh
	if (!HitResult.Component.IsValid() || HitResult.Component->GetClass() != UProceduralMeshComponent::StaticClass())
	{
		//UE_LOG(LogTemp, Warning, TEXT("Hit component is not the terrain mesh."));
		return false;
	}

	// Check if the physical material of the hit component is the expected type for terrain
	if (HitResult.PhysMaterial.IsValid() && HitResult.PhysMaterial->SurfaceType != SupportedSurfaceType)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Hit surface is not of the supported type."));
		return false;
	}

	// Check for height and slope constraints
	float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.ImpactNormal, FVector::UpVector)));
	bool isSuitable = HitResult.Location.Z >= MinHeightAboveTerrain && SlopeAngle <= GroundSlopeAngleMax;

	//UE_LOG(LogTemp, Log, TEXT("Checking location suitability: Height=%f, SlopeAngle=%f, IsSuitable=%s"),
		//HitResult.Location.Z, SlopeAngle, isSuitable ? TEXT("True") : TEXT("False"));

	return isSuitable;
}

//********************//
// Goal setting //
//********************//

void AWorldGenerator::SetGoalLocation()
{
	FVector PlayerLocation = GetPlayerLocation();
	ATGGameMode* MyGameMode = Cast<ATGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

	// Configuration for trace parameters and fallback logic
	FCollisionQueryParams TraceParams(FName(TEXT("GoalLocationTrace")), true);
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.AddIgnoredComponent(seaMesh);

	
	
	bool bSuitableLocationFound = false;
	int NumTries = 250; // Number of attempts to find a suitable location
	float TargetDistance = 2000; // Distance for the goal location from the player

	for (int i = 0; i < NumTries; i++)
	{
		// Random direction and goal location setup
		FVector GoalDirection = FMath::VRand();
		GoalDirection.Z = 0;
		GoalDirection.Normalize();

		FVector GoalLocation = PlayerLocation + (GoalDirection * TargetDistance);
		FVector Start = GoalLocation + FVector(0, 0, 500);
		FVector End = GoalLocation - FVector(0, 0, 500);
		FHitResult HitResult;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams))
		{
			if (HitResult.Component.IsValid() && HitResult.Component->IsA<UProceduralMeshComponent>())
			{
				FVector AdjustedGoalLocation = HitResult.Location + FVector(0, 0, 100);
				if (IsPathValid(PlayerLocation, AdjustedGoalLocation))
				{
					SpawnHealthItemAtGoalLocation(AdjustedGoalLocation);
					MyGameMode->GoalLocations.Add(AdjustedGoalLocation); 
					bSuitableLocationFound = true;
					break;
				}
			}
		}
	}

	if (!bSuitableLocationFound)
	{
		//UE_LOG(LogTemp, Warning, TEXT("No suitable goal location found after multiple attempts."));
		
	}
}

void AWorldGenerator::SetGoalLocation2()
{
	FVector PlayerLocation = GetPlayerLocation();
	ATGGameMode* MyGameMode = Cast<ATGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

	// Configuration for trace parameters and fallback logic
	FCollisionQueryParams TraceParams(FName(TEXT("GoalLocationTrace")), true);
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.AddIgnoredComponent(seaMesh);


	FVector GoalLocation2;
	bool bSuitableLocationFound = false;
	int NumTries = 250; // Number of attempts to find a suitable location
	float TargetDistance = 2000; // Distance for the goal location from the player

	for (int i = 0; i < NumTries; i++)
	{
		// Random direction and goal location setup
		FVector GoalDirection2 = FMath::VRand();
		GoalDirection2.Z = 0;
		GoalDirection2.Normalize();

		GoalLocation2 = PlayerLocation + (GoalDirection2 * TargetDistance);
		FVector Start = GoalLocation2 + FVector(0, 0, 500);
		FVector End = GoalLocation2 - FVector(0, 0, 500);
		FHitResult HitResult;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams))
		{
			if (HitResult.Component.IsValid() && HitResult.Component->IsA<UProceduralMeshComponent>())
			{
				FVector AdjustedGoalLocation = HitResult.Location + FVector(0, 0, 100);
					if (IsPathValid(PlayerLocation, AdjustedGoalLocation))
					{
						SpawnHealthItemAtGoalLocation(AdjustedGoalLocation);
						MyGameMode->GoalLocations.Add(AdjustedGoalLocation);
						bSuitableLocationFound = true;
						break;
					}
			}
		}
	}

	if (!bSuitableLocationFound)
	{
		//_LOG(LogTemp, Warning, TEXT("No suitable goal location 2 found after multiple attempts."));
	
	}
}

void AWorldGenerator::SetGoalLocation3()
{
	FVector PlayerLocation = GetPlayerLocation();
	ATGGameMode* MyGameMode = Cast<ATGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

	// Configuration for trace parameters and fallback logic
	FCollisionQueryParams TraceParams(FName(TEXT("GoalLocationTrace")), true);
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.AddIgnoredComponent(seaMesh);

	FVector GoalLocation3;
	bool bSuitableLocationFound = false;
	int NumTries = 250; // Number of attempts to find a suitable location
	float TargetDistance = 2500; // Distance for the goal location from the player

	for (int i = 0; i < NumTries; i++)
	{
		// Random direction and goal location setup
		FVector GoalDirection3 = FMath::VRand();
		GoalDirection3.Z = 0;
		GoalDirection3.Normalize();

		GoalLocation3 = PlayerLocation + (GoalDirection3 * TargetDistance);
		FVector Start = GoalLocation3 + FVector(0, 0, 500);
		FVector End = GoalLocation3 - FVector(0, 0, 500);
		FHitResult HitResult;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams))
		{
			if (HitResult.Component.IsValid() && HitResult.Component->IsA<UProceduralMeshComponent>())
			{
				FVector AdjustedGoalLocation = HitResult.Location + FVector(0, 0, 100);
				if (IsPathValid(PlayerLocation, AdjustedGoalLocation))
				{
					SpawnHealthItemAtGoalLocation(AdjustedGoalLocation);
					MyGameMode->GoalLocations.Add(AdjustedGoalLocation);
					bSuitableLocationFound = true;
					break;
				}
			}
		}
	}

	if (!bSuitableLocationFound)
	{
	//UE_LOG(LogTemp, Warning, TEXT("No suitable goal location 3 found after multiple attempts."));
		
	}
}

void AWorldGenerator::SetGoalLocationsComplete()
{


	SetGoalLocation();
	SetGoalLocation2();
	SetGoalLocation3();
}
	


//********************//
// Sea //
//********************//
void AWorldGenerator::RelocateSea()
{
	if (seaMesh)
	{
		FVector PlayerLocation = GetPlayerLocation();
		FVector SeaLocation = seaMesh->GetComponentLocation();

		// Calculate 2D distance between the sea and the player
		float Distance = FVector::Dist2D(SeaLocation, PlayerLocation);

		if (Distance > Relocate)
		{
			// Only adjust the X and Y coordinates based on the PlayerLocation, Z is set to seaLevel
			FVector NewSeaLocation = FVector(PlayerLocation.X, PlayerLocation.Y, seaLevel);
			seaMesh->SetWorldLocation(NewSeaLocation, false, (FHitResult*)nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void AWorldGenerator::UpdateSeaParameters()
{

	if (seaMesh)
	{
		// Set sea visibility based on 'EnableSea' variable
		seaMesh->SetVisibility(enableSea, true);

		if (enableSea) // Only update parameters if sea is enabled
		{
			FVector CurrentLocation = seaMesh->GetComponentLocation();

			// Set new world location
			seaMesh->SetWorldLocation(FVector(CurrentLocation.X, CurrentLocation.Y, seaLevel), false, (FHitResult*)nullptr, ETeleportType::TeleportPhysics);

			// Set new world scale
			seaMesh->SetWorldScale3D(FVector(seaScale, seaScale, seaScale));

			// Update material parameter 
			if (UMaterialInstanceDynamic* MaterialInstance = Cast<UMaterialInstanceDynamic>(seaMesh->GetMaterial(0)))
			{
				// Set scalar parameter value
				MaterialInstance->SetScalarParameterValue(FName("SeaLevel"), seaLevel);
				MaterialInstance->SetScalarParameterValue(FName("SeaScale"), seaScale);
			}
		}

	}
}

void AWorldGenerator::ActorsToMove()
{
	for (AActor* Actor : ToMoveActors)
	{
		if (Actor) // Is Valid check
		{
			FVector ActorLocation = Actor->GetActorLocation();
			FVector PlayerLocation = GetPlayerLocation();

			// Calculate 2D distance between the actor and the player
			float Distance = FVector::Dist2D(ActorLocation, PlayerLocation);

			if (Distance > Relocate)
			{
				Actor->SetActorLocation(FVector(ActorLocation.X, ActorLocation.Y, ActorLocation.Z), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}
}