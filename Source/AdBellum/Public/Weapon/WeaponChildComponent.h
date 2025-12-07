// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EBBarrel.h"
#include "Components/ChildActorComponent.h"
#include "SimpleWeapon.h"
#include "WeaponChildComponent.generated.h"


UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API UWeaponChildActorComponent : public UChildActorComponent
{
	GENERATED_BODY()

public:
	UWeaponChildActorComponent(const FObjectInitializer& Initializer);

	//void UWeaponChildActorComponent::PostInitializeComponents();

	virtual void BeginPlay() override;

	void Trigger(bool Value);

	//used to find look for ADS arrow comp
	FVector GetCalibrationVector(float Distance);

	TObjectPtr<ASimpleWeapon> ControlledWeapon;
};