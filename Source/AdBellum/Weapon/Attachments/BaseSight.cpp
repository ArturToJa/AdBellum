// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSight.h"
#include "Kismet/KismetMathLibrary.h"
#include <Kismet/KismetSystemLibrary.h>
#include "BaseScope.h"
#include <Net/UnrealNetwork.h>

// Sets default values
ABaseSight::ABaseSight()
{
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	PrimaryActorTick.bCanEverTick = false;
	SightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sight Mesh"));
	SightView = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sight View"));
	ADS = CreateDefaultSubobject<USceneComponent>(TEXT("ADS Target"));

	SightMesh->SetupAttachment(RootComponent);
	SightView->SetupAttachment(RootComponent);
	ADS->SetupAttachment(RootComponent);

	SightArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ViewArrow"));
	SightArrow->SetupAttachment(SightView);
	bReplicates = false;
	bAlwaysRelevant = true;
}

void ABaseSight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseSight, BaseMouseSensitivity);
}

void ABaseSight::CalibrateSight(FVector TargetLocation)
{
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(SightArrow->GetRelativeLocation(), TargetLocation);
	//UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(TargetRotation.Yaw).Append("<--- Calculated YAW value "), true, true);
	//SightView->AddRelativeRotation(FRotator(0.0f, TargetRotation.Yaw,0.0f),true, nullptr, ETeleportType::TeleportPhysics);
	RootComponent->SetRelativeRotation(FRotator(TargetRotation.Pitch, 0.0f,0.0f),true, nullptr, ETeleportType::TeleportPhysics);
	//UKismetSystemLibrary::PrintString(GetWorld(), "SIGHT CALIBRATED", true, true);
}

void ABaseSight::NotifyAim(bool bIsAiming)
{
	return;
}

float ABaseSight::GetBaseSensitivity()
{
	return BaseMouseSensitivity;
}

USightChildActorComponent::USightChildActorComponent(const FObjectInitializer& Initializer):UChildActorComponent(Initializer)
{
	SetIsReplicatedByDefault(true);
}

void USightChildActorComponent::BeginPlay()
{
	Super::BeginPlay();
}

ABaseSight* USightChildActorComponent::GetSightActor() const
{
	return Cast<ABaseSight>(this->GetChildActor());
}

void USightChildActorComponent::CalibrateSightActor(FVector TargetLocation)
{
	if (GetSightActor())
	{
		GetSightActor()->CalibrateSight(TargetLocation);
	}
}