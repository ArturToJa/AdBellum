// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseAmmo.h"
#include "Weapon/EBDamageType.h"
#include "Character/ALSBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Library/BodyPartBoneMapping.h"
#include "Library/ImpactEffectDataStruct.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "Unit/BaseUnit.h"
#include "ITargetable.h"
#include "Engine/DecalActor.h"
#include "Components/DecalComponent.h"
#include "Perception/AISense_Damage.h"
#include <Kismet/KismetStringLibrary.h>

ABaseAmmo::ABaseAmmo(const FObjectInitializer& ObjectInitializer)
{
	InitialLifeSpan = 10.0f;
}

void ABaseAmmo::OnImpact_Implementation(bool Ricochet, bool PassedThrough, FVector Location, FVector IncomingVelocity,
	FVector Normal, FVector ExitLocation, FVector ExitVelocity, FVector Impulse, float PenetrationDepth,
	AActor* Actor, USceneComponent* Component, FName BoneName, UPhysicalMaterial* PhysMaterial, FHitResult HitResult) 
{
	
	EPhysicalSurface HitSurfaceEnum = PhysMaterial->SurfaceType;
	if (HitSurfaceEnum) 
	{
		if (!HitSurfaceEnum == SurfaceType_Default)
		{
			Server_PlayEffects(HitSurfaceEnum, Location, IncomingVelocity);
		}
		if (SpawnDecal)
		{
			ADecalActor* decal = GetWorld()->SpawnActor<ADecalActor>(Location, UKismetMathLibrary::Conv_VectorToRotator(IncomingVelocity));
			if (decal)
			{
				decal->SetDecalMaterial(DecalMaterial);
				decal->SetLifeSpan(15.0f);
				decal->GetDecal()->DecalSize = FVector(10.0f, 3.0f, 3.0f);
			}
		}
	}
	if (Actor) 
	{
		DealDamage(Actor, HitResult, IncomingVelocity);
	}
}

void ABaseAmmo::Server_PlayEffects_Implementation(EPhysicalSurface HitSurfaceEnum, FVector Location, FVector IncomingVelocity)
{
	Multicast_PlayEffects(HitSurfaceEnum, Location, IncomingVelocity);
}

void ABaseAmmo::Multicast_PlayEffects_Implementation(EPhysicalSurface HitSurfaceEnum, FVector Location, FVector IncomingVelocity)
{
	PlayEffects(HitSurfaceEnum, Location, IncomingVelocity);
}

void ABaseAmmo::PlayEffects(EPhysicalSurface HitSurfaceEnum, FVector Location, FVector IncomingVelocity)
{
	FImpactEffectDataStruct ImpactEffects = EffectsLibrary.GetImpactEffectData(HitSurfaceEnum);
	const FVector AdjustedScale = EmitterBaseScale.operator*(IncomingVelocity.Length() / MuzzleVelocityMax);
	if (ImpactEffects.ImpactEffect && (FMath::RandRange(1,6) == 1))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffects.ImpactEffect, Location, UKismetMathLibrary::Conv_VectorToRotator(IncomingVelocity), AdjustedScale);
	}
	if (ImpactEffects.ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactEffects.ImpactSound, Location, AdjustedScale.X);
	}
}

void ABaseAmmo::DealDamage(AActor* DamagedActor, FHitResult HitResult, FVector IncomingVelocity)
{
	if (DamagedActor) 
	{
		if (DamagedActor->IsA<ABaseUnit>())
		{
			//UKismetSystemLibrary::PrintString(GetWorld(), HitResult.BoneName.ToString(), true, true);
			DealHumanDamage(IncomingVelocity, FindBodyPart(HitResult.BoneName), DamagedActor);
		}
		else
		{
			float Damage = CalculateEnergy(IncomingVelocity);
			UGameplayStatics::ApplyDamage(DamagedActor, Damage, GetInstigatorController(), GetOwner()->GetOwner(), UEBDamageType::StaticClass());
		}
	}
}

void ABaseAmmo::DealHumanDamage(FVector IncomingVelocity, EBodyPart BodyPart, AActor* DamagedActor)
{
	float BodyPartMultiplier = 1.0f;
	float ArmourMultiplier = IITargetable::Execute_GetArmourParamValueForBodyPart(DamagedActor, BodyPart);
	TSubclassOf<UEBDamageType> DamageTypeClass = UEBDamageType::StaticClass();
	//maybe implement all these values to GetArmourParamValueForBodyPart
	
	switch (BodyPart)
	{
	case EBodyPart::HEAD:
		{
			BodyPartMultiplier = 2.0f;
			break;
		}
	case EBodyPart::TORSO:
		{
			BodyPartMultiplier = 1.0f;
			break;
		}
	case EBodyPart::ARMS:
		{
			BodyPartMultiplier = 0.2f;
			break;
		}
	case EBodyPart::LEGS:
		{
			BodyPartMultiplier = 0.15f;
			break;
		}
	}


	float Damage = (-1 * CalculateEnergy(IncomingVelocity) * BodyPartMultiplier * ArmourMultiplier);
	//PrintDebugInfo(BodyPart, BodyPartMultiplier, ArmourMultiplier, IncomingVelocity);
	UGameplayStatics::ApplyDamage(DamagedActor, Damage, GetInstigatorController(), GetOwner()->GetOwner(), DamageTypeClass);
}

void ABaseAmmo::PrintDebugInfo(EBodyPart BodyPart, float BodyPartMultiplier, float ArmourMultiplier, FVector IncomingVelocity)
{
	//debug bone mapping
	//UKismetSystemLibrary::PrintString(GetWorld(), UEnum::GetValueAsString(BodyPart), true, true);
	UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(BodyPartMultiplier).Append("<--- BodyPartMultiplier "), true, true);
	UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(ArmourMultiplier).Append("<--- ArmourMultiplier "), true, true);
	UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(CalculateEnergy(IncomingVelocity)).Append("<--- ENERGY"), true, true);
	UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(-1 * CalculateEnergy(IncomingVelocity) * BodyPartMultiplier * ArmourMultiplier).Append("<--- DAMAGE"), true, true);
}

EBodyPart ABaseAmmo::FindBodyPart(FName BoneName)
{
	return FBodyPartBoneMapping::GetBodyPartFromBoneName(BoneName);
}

float ABaseAmmo::CalculateEnergy(FVector CurrentVelocity)
{
	return 0.5f * Mass * FMath::Pow(0.01 * Velocity.Length(), 2);
}

