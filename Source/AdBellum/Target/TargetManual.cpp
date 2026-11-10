// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetManual.h"
#include "TargetField.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ATargetManual::ATargetManual()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("defaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Hierarchy, meshes and transforms below match the values captured from the live Blueprint instance before
	// reparenting, so the hand-tuned target-field scaling isn't lost.
	MainTarget = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("mainTarget"));
	MainTarget->SetupAttachment(RootComponent);
	MainTarget->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	MainTarget->SetRelativeScale3D(FVector(2.4f, 2.4f, 2.4f));
	// Purely a visual backdrop (the printed target face) - it must not physically block shots, or they stop
	// here instead of ever reaching the thin ATargetField discs layered on its face.
	MainTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> MainTargetMeshFinder(TEXT("/Game/Added_content/WorldBuilding/Military_range_pack/Shoot_targets_decals/Meshes/SM_military_target_05.SM_military_target_05"));
		if (MainTargetMeshFinder.Succeeded())
		{
			MainTarget->SetStaticMesh(MainTargetMeshFinder.Object);
		}
	}

	ScoreText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("score"));
	ScoreText->SetupAttachment(MainTarget);
	ScoreText->SetRelativeLocation(FVector(-5.766423f, 124.915015f, 125.110342f));
	ScoreText->SetRelativeScale3D(FVector(8.0f, 8.0f, 8.0f));
	ScoreText->SetText(FText::FromString(TEXT("0")));
	ScoreText->SetTextRenderColor(FColor(255, 0, 231));
	ScoreText->SetWorldSize(26.0f);
	ScoreText->SetHorizontalAlignment(EHTA_Left);
	ScoreText->SetVerticalAlignment(EVRTA_TextCenter);

	DamageText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("damage"));
	DamageText->SetupAttachment(MainTarget);
	DamageText->SetRelativeLocation(FVector(-5.766423f, -28.146109f, 71.761755f));
	DamageText->SetRelativeScale3D(FVector(2.5f, 2.5f, 2.5f));
	DamageText->SetText(FText::FromString(TEXT("0")));
	DamageText->SetTextRenderColor(FColor(255, 52, 0));
	DamageText->SetWorldSize(26.0f);
	DamageText->SetHorizontalAlignment(EHTA_Left);
	DamageText->SetVerticalAlignment(EVRTA_TextCenter);

	Middle = CreateDefaultSubobject<USceneComponent>(TEXT("middle"));
	// Attached to MainTarget's own TargetCenter socket rather than a manually-placed offset, so it tracks the
	// mesh's actual scoring center.
	Middle->SetupAttachment(MainTarget, TEXT("TargetCenter"));
	// Cancels MainTarget's own 2.4x scale so the field markers below are sized in absolute, not visual-mesh-
	// relative, units.
	Middle->SetRelativeScale3D(FVector(1.0f / 2.4f, 1.0f / 2.4f, 1.0f / 2.4f));

	// Field markers "1".."10" - the name is parsed as each spawned ATargetField's point value (see BeginPlay).
	// Smaller discs (higher-numbered markers) sit closer to the camera so they don't z-fight with the larger
	// ones behind them, matching a classic bullseye: "10" is the smallest/innermost ring.
	static const float MarkerScaleXY[10] = { 1.09f, 0.98f, 0.88f, 0.77f, 0.66f, 0.55f, 0.44f, 0.33f, 0.22f, 0.12f };
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	FieldMarkers.SetNum(10);
	for (int32 i = 0; i < 10; ++i)
	{
		const FName MarkerName(*FString::FromInt(i + 1));
		UStaticMeshComponent* Marker = CreateDefaultSubobject<UStaticMeshComponent>(MarkerName);
		Marker->SetupAttachment(Middle);
		Marker->SetRelativeLocation(FVector(0.1f + 0.05f * i, 0.0f, 0.0f));
		Marker->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
		Marker->SetRelativeScale3D(FVector(MarkerScaleXY[i], MarkerScaleXY[i], 0.01f));
		if (CylinderMeshFinder.Succeeded())
		{
			Marker->SetStaticMesh(CylinderMeshFinder.Object);
		}
		// Placement/scale templates only - the ATargetField spawned from each (see BeginPlay) is what's
		// actually visible/hittable.
		Marker->SetVisibility(false);
		FieldMarkers[i] = Marker;
	}

	bReplicates = true;
	NetDormancy = DORM_Awake;
}

void ATargetManual::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATargetManual, RepScore);
	DOREPLIFETIME(ATargetManual, RepDamage);
}

void ATargetManual::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		for (UStaticMeshComponent* Marker : FieldMarkers)
		{
			if (!Marker)
			{
				continue;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			ATargetField* Field = GetWorld()->SpawnActor<ATargetField>(ATargetField::StaticClass(), Marker->GetComponentTransform(), SpawnParams);
			if (Field)
			{
				Field->OwningTarget = this;
				Field->Points = FCString::Atoi(*Marker->GetName());
				// The marker is just a hidden placement/scale template (see constructor) - gives the field the
				// same collision shape, though the field itself stays invisible (see ATargetField's constructor).
				if (Field->Mesh)
				{
					Field->Mesh->SetStaticMesh(Marker->GetStaticMesh());
				}
				SpawnedFields.Add(Field);
			}
		}
	}
}

void ATargetManual::NotifyScore(int32 Points, float Damage)
{
	if (!HasAuthority())
	{
		return;
	}

	RepScore = Points;
	RepDamage = Damage;
	UpdateScoreDisplay();
	UpdateDamageDisplay();
}

void ATargetManual::OnRep_RepScore()
{
	UpdateScoreDisplay();
}

void ATargetManual::OnRep_RepDamage()
{
	UpdateDamageDisplay();
}

void ATargetManual::UpdateScoreDisplay()
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::AsNumber(RepScore));
	}
}

void ATargetManual::UpdateDamageDisplay()
{
	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(RepDamage));
	}
	GetWorldTimerManager().SetTimer(DamageClearTimerHandle, this, &ATargetManual::ClearDamageText, 1.0f, false);
}

void ATargetManual::ClearDamageText()
{
	if (DamageText)
	{
		DamageText->SetText(FText::GetEmpty());
	}
}

bool ATargetManual::IsAlive_Implementation()
{
	return true;
}

int32 ATargetManual::GetTeamIndex_Implementation()
{
	return 99;
}

FVector ATargetManual::GetChestLocation_Implementation()
{
	return Middle ? Middle->GetComponentLocation() : GetActorLocation();
}
