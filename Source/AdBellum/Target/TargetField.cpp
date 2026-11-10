// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetField.h"
#include "TargetManual.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ATargetField::ATargetField()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetNotifyRigidBodyCollision(true);
	// Invisible hit-volume only - MainTarget's own printed texture already shows the scoring rings; this mesh
	// (copied from the marker purely for its collision shape, see ATargetManual::BeginPlay) must never render.
	// Also sidesteps StaticMesh not being a replicated property: a client would otherwise never see the mesh
	// the server assigns at spawn, while the server's own copy stayed visible as an unwanted gray disc.
	Mesh->SetVisibility(false);

	bReplicates = true;
	SetReplicateMovement(true);
	SetCanBeDamaged(true);
}

void ATargetField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATargetField, OwningTarget);
}

void ATargetField::BeginPlay()
{
	Super::BeginPlay();

	// Damage (radial from ATGM, point from bullets) is only ever applied by the server - see the project's own
	// weapon code - so only the server needs to react to it here.
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ATargetField::OnAnyDamageReceived);
	}
}

void ATargetField::OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (OwningTarget)
	{
		OwningTarget->NotifyScore(Points, Damage);
	}
}
