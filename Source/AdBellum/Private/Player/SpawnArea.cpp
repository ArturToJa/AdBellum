// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SpawnArea.h"
#include "System/AdBellumPlayerState.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Player/IPlayer.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ASpawnArea::ASpawnArea()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	UserTransformsCounter = 0;
}

// Called when the game starts or when spawned
void ASpawnArea::BeginPlay()
{
	Super::BeginPlay();
}

void ASpawnArea::OnConstruction(const FTransform& Transform)
{
	InitializePlayerSpawnPoint(Transform);
	GenerateCorners(Transform);
}

FTransform ASpawnArea::GetNextTransform()
{
	FTransform NextTransform = SpawnTransforms[UserTransformsCounter];
	UserTransformsCounter++;
	if (UserTransformsCounter >= SpawnTransforms.Num())
	{
		UserTransformsCounter = 0;
	}
	return NextTransform;
}

int ASpawnArea::GetPriority(int TeamIndex)
{
	return TeamId == TeamIndex;
}


void ASpawnArea::GenerateTransforms(int NumberOfUnits)
{
	if (NumberOfUnits == 0) return;

	UserTransformsCounter = 0;
	SpawnTransforms.Empty();
	SpawnTransforms.SetNum(NumberOfUnits, true);
	int TransformGeneratedCounter = 0;

	FQuat SpawnRotation = GetTransform().GetRotation();
	FVector BackwardsVector = GetActorForwardVector() * -1;
	FVector RightVector = GetActorRightVector();

	float NumberOfUnitsSquared = FMath::Sqrt((float)NumberOfUnits);
	int32 NumRows = FMath::CeilToInt(NumberOfUnitsSquared);
	int32 NumCols = FMath::RoundToInt(NumberOfUnitsSquared);

	float RowSpacing = FVector::Dist2D(TopLeftCorner, BottomLeftCorner) / NumRows;
	float ColsSpacing = FVector::Dist2D(TopLeftCorner, TopRightCorner)/ NumCols;

	FTransform SpawnTransform;
	SpawnTransform.SetRotation(SpawnRotation);
	SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

	// First row
	int32 NumberOfUnitsInFirstRow = NumberOfUnits % NumCols;
	if (NumberOfUnitsInFirstRow == 0)
	{
		NumberOfUnitsInFirstRow = NumCols;
	}
	float FirstRowSpacing = FVector::Dist2D(TopLeftCorner, TopRightCorner) / (NumberOfUnitsInFirstRow + 2);
	for (int Col = 1; Col <= NumberOfUnitsInFirstRow; ++Col)
	{
		FVector SpawnLocation = TopLeftCorner + RightVector * Col * FirstRowSpacing;

		FHitResult Hit;
		FCollisionObjectQueryParams ObjParams(ECollisionChannel::ECC_WorldStatic);
		FCollisionQueryParams ColParams;
		ColParams.AddIgnoredActor(this);
		GetWorld()->LineTraceSingleByObjectType(Hit, SpawnLocation + 500.f,
			SpawnLocation - 500.f,
			ObjParams, ColParams);

		if (Hit.bBlockingHit)
		{
			SpawnLocation = Hit.Location;
		}

		SpawnTransform.SetLocation(SpawnLocation);
		SpawnTransforms[TransformGeneratedCounter] = SpawnTransform;
		TransformGeneratedCounter++;
	}

	// All the remaining rows
	for (int Row = 1; Row < NumRows; ++Row)
	{
		for (int Col = 0; Col < NumCols; ++Col)
		{
			FVector SpawnLocation = TopLeftCorner + BackwardsVector * Row * RowSpacing + RightVector * Col * ColsSpacing;
			SpawnTransform.SetLocation(SpawnLocation);
			SpawnTransforms[TransformGeneratedCounter] = SpawnTransform;
			TransformGeneratedCounter++;
		}
	}
}

void ASpawnArea::InitializePlayerSpawnPoint(const FTransform& Transform)
{
	FVector Scale = BoxComponent->GetScaledBoxExtent();
	FVector NewLocation = Transform.GetLocation() - UKismetMathLibrary::GetForwardVector(Transform.Rotator()) * Scale.Y;
	NewLocation.Z += 2000.0f;
	RTSCameraSpawnTransform.SetLocation(NewLocation);
	FRotator Rotation = Transform.GetRotation().Rotator();
	Rotation.Pitch = -45.0f;;
	RTSCameraSpawnTransform.SetRotation(Rotation.Quaternion());
}

void ASpawnArea::GenerateCorners(const FTransform& Transform)
{
	FVector Scale = BoxComponent->GetScaledBoxExtent();
	FVector Location = Transform.GetLocation();
	FVector Forward = UKismetMathLibrary::GetForwardVector(Transform.Rotator());
	FVector Right = UKismetMathLibrary::GetRightVector(Transform.Rotator());
	FVector ForwardScaled = Forward * Scale.Y * 0.9;
	FVector RightScaled = Right * Scale.X * 0.9;
	TopLeftCorner = Location + ForwardScaled - RightScaled;
	TopRightCorner = Location + ForwardScaled + RightScaled;
	BottomLeftCorner = Location - ForwardScaled - RightScaled;
}

FTransform ASpawnArea::GetPlayerSpawnPoint()
{
	return RTSCameraSpawnTransform;
}