// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enterable.h"
#include "OrderSystem/Orderable.h"
#include "Interfaces/Customizable.h"
#include "Vehicle/Vehicle.h"
#include "Interfaces/Selectable.h"
#include "_Core/AVS_Vehicle.h"
#include "VehicleSeatComponent.h"
#include "VehicleStructs.h"
#include "Library/ALSADSInterface.h"
#include "Weapon/BaseWeapon.h"
#include "Weapon/WeaponChildComponent.h"
#include "BaseVehicle.generated.h"

class UNetworkComponent;
class ABaseWeapon;
class UGunnerSeatComponent;

UCLASS()
class ADBELLUM_API ABaseVehicle : public AAVS_Vehicle, public IEnterable, public IFormable, public ISelectable, public IOrderable, public ICustomizable, public IVehicle, public IALSInputInterface, public IALSADSInterface
{
	GENERATED_BODY()
public:

	ABaseVehicle(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNetworkComponent> NetworkComponent;

	//possession

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	AUnitAIController* BackupAIController = nullptr;

	void PossessByAIController();

	// SEATS

	// Seat of the driver, part of every vehicle: place it in the viewport and list it in an access point
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UVehicleSeatComponent> DriverSeat;

	UFUNCTION(BlueprintCallable, Category = "Seats")
	UVehicleSeatComponent* GetSeatWithRole(EALSStationaryRole StationaryRole) const;

	// Every gunner seat of the vehicle, each one has its own weapons and its own aim
	UFUNCTION(BlueprintCallable, Category = "Seats")
	TArray<UGunnerSeatComponent*> GetGunnerSeats() const;

	// DAMAGE
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	float HP = 5000.0f;

	UFUNCTION()
	void OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle - General")
	TObjectPtr<USkeletalMeshComponent> VehicleSkeletalMesh;

	// MESH

	// Bones of the vehicle skeletal mesh that are not drawn, together with the bones below them
	// (hiding turret_jnt also hides the gun). Use it for parts replaced by a more detailed weapon or
	// turret model. Only drawing is switched off: the bone still moves, so whatever is attached to it
	// (weapon, seat) stays where it belongs and keeps its size.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TArray<FName> HiddenBones;

	// WHEELS

	// Wheels created on the skeletal mesh's wheel bones when the vehicle is constructed.
	// Empty - found by the standard bone names lf_wheel_N_jnt / rt_wheel_N_jnt (N - axle, 1 is the front one)
	// and given their roles by the axle lists below. Fill it to decide every wheel by hand instead.
	// Wheel components added in the blueprint are left alone either way.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wheels")
	TArray<FVehicleWheelSetup> WheelSetups;

	// Suspension, tire and brake settings of every created wheel
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wheels")
	FAVS1_Wheel_Config DefaultWheelConfig;

	// Axles that steer, used when wheels are found by bone name
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wheels|Auto Detect")
	TArray<int32> SteerableAxles = { 1 };

	// Axles that receive torque, empty - all of them
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wheels|Auto Detect")
	TArray<int32> DrivenAxles;

	// Axles the handbrake holds, empty - the last one
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wheels|Auto Detect")
	TArray<int32> HandbrakeAxles;

	// WheelSetups, or what the skeleton's standard wheel bones give when WheelSetups is empty
	UFUNCTION(BlueprintCallable, Category = "Wheels")
	TArray<FVehicleWheelSetup> GetEffectiveWheelSetups() const;

	// IEnterable interface
	virtual UVehicleAccessPoint* TryBookSeat_Implementation(AALSBaseCharacter* InUnit) override;
	virtual TArray<UVehicleAccessPoint*> GetAllAccessPoints_Implementation() override;

	// IFormable interface

	virtual void SetFormation_Implementation(ABaseFormation* Formation) override;
	virtual ABaseFormation* GetFormation_Implementation() override;

	// ISelectable interface
	virtual void SetSelectionCircle_Implementation(bool Visible) override;
	virtual class UOrdersManager* GetOrdersManagerComponent_Implementation() override;
	virtual bool IsAlive_Implementation();
	virtual void SetOwningPlayer_Implementation(AActor* Player) override;
	virtual AActor* GetOwningPlayer_Implementation() override;
	virtual void SetTeamIndex_Implementation(int32 Index) override;
	virtual int32 GetTeamIndex_Implementation() override;
	virtual void SetInstanceIndex_Implementation(int32 Index) override;
	virtual int32 GetInstanceIndex_Implementation() override;
	virtual int GetUnitType_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOrdersManager> OrdersManagerComponent;

	//Input interface, called on the machine of the player driving the vehicle (the driver possesses it)

	virtual void ForwardMovementAction_Implementation(float Value) override;

	virtual void ForwardMovementActionCompleted_Implementation() override;

	virtual void RightMovementAction_Implementation(float Value) override;

	virtual void RightMovementActionCompleted_Implementation() override;

	// Handbrake, held while the action keeps triggering
	virtual void JumpAction_Implementation(bool bValue) override;

	virtual void InteractionAction_Implementation() override;

	UFUNCTION(Server, Reliable)
	void Server_InteractionAction();

	//virtual void PrimarySelectionAction_Implementation() override;
	//virtual void SecondarySelectionAction_Implementation() override;
	//virtual void ThirdSelectionAction_Implementation() override;

	//Vehicle interface
	virtual float GetSteeringValue_Implementation() override;

	//ADS Interface, fallbacks only: a gunner's ADS comes from the gunner's weapon (ASimpleWeapon)
	virtual FVector getADSTarget_Implementation() override;

	virtual FRotator getADSRotation_Implementation() override;

	virtual FTransform getCameraTransform_Implementation(EALSStationaryRole StationaryRole) override;

	//orderable

	//virtual void Stop_Implementation(FVector TargetPosition) override;
	//virtual void MoveOrder_Implementation(FVector TargetPosition) override;
	//virtual void AttackTarget_Implementation(UObject* TargetObject) override;
	//virtual void AttackLocation_Implementation(FVector TargetPosition) override;
	//virtual void DoCrouch_Implementation() override;
	//virtual void DoCrawl_Implementation() override;
	//virtual void DoStandUp_Implementation() override;
	//virtual TScriptInterface<IIWeapon> GetWeapon_Implementation() override;

	//customizable
	//virtual void GetSkeletalMeshFromDefinition_Implementation(const EBodyPart BodyPart, UPARAM(ref) const FMeshCreatorDataInputStruct& MeshDefinition,
	//	FMeshCreatorOutputStruct& Output) const override;
	//virtual void ConfigureUnit_Implementation(const FMeshCreatorPrefabStruct& UnitPrefab) override;
	//virtual void ConfigureWeapon_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum) override;

	UPROPERTY(Replicated)
	int32 TeamIndex;
	UPROPERTY(Replicated)
	AActor* PlayerPtr;
	int32 SelectionCircleIndex;
	UPROPERTY(Replicated)
	ABaseFormation* OwningFormation;

private:
	// Stops drawing every bone listed in HiddenBones
	void ApplyHiddenBones();

	// Creates a raycast UAVS_Wheel on the bone of every effective wheel setup
	void BuildWheelsFromSetup();

	// Throttle, brake and steering back to zero
	void ReleaseDriverInputs();

	void ReleaseHandbrake();

	// JumpAction only reports the key being held, the handbrake lets go when the reports stop
	FTimerHandle HandbrakeReleaseTimer;
};
