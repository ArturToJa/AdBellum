// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Character/ALSInteractionInterface.h"
#include "Unit/BaseUnit.h"
#include "VehicleAccessPoint.generated.h"

class AVRWheeledVehicle;
class UVehicleSeatComponent;
class AALSBaseCharacter;
/**
 * 
 */
UCLASS(Blueprintable, ClassGroup = (VehicleComponent), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API UVehicleAccessPoint : public UBoxComponent, public IALSInteractionInterface
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	// ALSInteractionInterface++
	virtual	void Interact_Implementation(AALSBaseCharacter* InUnit) override;
	virtual	float InteractRange_Implementation() override;
	virtual	FVector InteractLocation_Implementation() override;
	// ALSInteractionInterface--

	UFUNCTION(BlueprintCallable)
	UVehicleSeatComponent* GetAvailableSeat(AALSBaseCharacter* InUnit);

	UVehicleSeatComponent* GetSeatWithRole(EALSStationaryRole StationaryRole);

	//UFUNCTION()
	//void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//UFUNCTION()
	//void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable)
	void Exit(AALSBaseCharacter* ExitingChar);

	UFUNCTION(BlueprintCallable)
	bool BookSeat(AALSBaseCharacter* InUnit);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Meta = (UseComponentPicker))
	TArray<FComponentReference> SeatArray;
	
private:
	AActor* OwnerActor;
};
