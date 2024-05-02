#include "Spawner.h"

// Sets default values
ASpawner::ASpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 3;

}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();


}

// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTiles();
}

FVector ASpawner::GetPlayerCell()
{
	FVector Location = FVector::Zero();
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	AActor* Player = PlayerController->GetPawn();
	if (Player)
	{
		Location = Player->GetActorLocation();
	}
	Location = Location / CellSize;
	Location = FVector(FMath::RoundToInt(Location.X), FMath::RoundToInt(Location.Y), FMath::RoundToInt(Location.Z)) * CellSize;

	return Location;
}

void ASpawner::UpdateTiles()
{
	RemoveFarTiles();
	FVector Origin = GetPlayerCell();

	for (int Y = CellCount * (-.5f); Y <= CellCount * (0.5f); Y++)
	{
		for (int X = CellCount * (-.5f); X <= CellCount * (0.5f); X++)
		{
			FVector TileCenter = Origin + FVector(X, Y, 0) * CellSize;

			FHitResult Hit;
			FCollisionQueryParams CollisionParams;
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			AActor* Player = PlayerController->GetPawn();
			CollisionParams.AddIgnoredActor(Player);

			bool bHit = GetWorld()->LineTraceSingleByChannel(
				Hit,
				TileCenter + FVector::UpVector * TraceDistance,
				TileCenter - FVector::UpVector * TraceDistance,
				ECC_Visibility,
				CollisionParams
			);

			if (bHit)
			{
				if (!SpawnedTiles.Contains(FVector2D(TileCenter.X, TileCenter.Y)))
				{
					//DrawDebugBox(GetWorld(), Hit.Location, FVector(1, 1, 1) * CellSize * .5f, FColor::Red, false, 5);
					SpawnedTiles.Add(FVector2D(TileCenter.X, TileCenter.Y));
					UpdateTile(Hit.Location);
				}

			}
		}
	}

}

void ASpawner::UpdateTile(const FVector TileCenter)
{
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.bReturnPhysicalMaterial = true;
	bool bHit;

	for (int Y = CellSize * (-.5f); Y <= CellSize * (.5f); Y += SubCellSize)
	{
		for (int X = CellSize * (-.5f); X < CellSize * (.5f); X += SubCellSize)
		{
			FVector SubCellLocation = TileCenter + FVector(X + FMath::RandRange(-SubCellRandomOffset, SubCellRandomOffset), Y + FMath::RandRange(-SubCellRandomOffset, SubCellRandomOffset), 0);
			bHit = GetWorld()->LineTraceSingleByChannel(
				Hit,
				SubCellLocation + FVector::UpVector * TraceDistance,
				SubCellLocation - FVector::UpVector * TraceDistance,
				ECC_Visibility,
				CollisionParams
			);
			if (bHit)
			{
				//DrawDebugLine(GetWorld(), Hit.Location + FVector::UpVector * 100, Hit.Location, FColor::Green, false, 5);
				SpawnObject(Hit, TileCenter);
			}
		}
	}
}

void ASpawner::SpawnObject(const FHitResult Hit, const FVector ParentTileCenter)
{

}

void ASpawner::RemoveFarTiles()
{
	FVector PlayerCell = GetPlayerCell();
	TArray<FVector2D> SpawnedTilesCopy;
	SpawnedTilesCopy.Append(SpawnedTiles);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	AActor* Player = PlayerController->GetPawn();
	if (!Player)
	{
		return;
	}

	for (int TileIndex = 0; TileIndex < SpawnedTilesCopy.Num(); TileIndex++)
	{
		FVector2D RelativeTileLocation = SpawnedTilesCopy[TileIndex] - FVector2D(PlayerCell);
		if (
			FMath::Abs(RelativeTileLocation.X) > CellCount * .5f * CellSize ||
			FMath::Abs(RelativeTileLocation.Y) > CellCount * .5f * CellSize
			)
		{
			FVector TileCenter = FVector(SpawnedTilesCopy[TileIndex], PlayerCell.Z);
			FHitResult Hit;

			if (PerformLineTrace(TileCenter + FVector::UpVector * TraceDistance, TileCenter - FVector::UpVector * TraceDistance, Hit))
			{
				//DrawDebugBox(GetWorld(), Hit.Location, FVector(1, 1, 1) * CellSize * .5f, FColor::Blue, false, 5);
				RemoveTile(Hit.Location);
				SpawnedTiles.Remove(SpawnedTilesCopy[TileIndex]);
			}
		}
	}
}

bool ASpawner::PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit) const
{
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		ECC_Visibility,
		CollisionParams
	);
}




void ASpawner::RemoveTile(const FVector TileCenter)
{

}