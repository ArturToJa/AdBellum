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
#include "UI/SelectionMenuProvider.h"
#include "BaseVehicle.generated.h"

class UNetworkComponent;
class ABaseWeapon;
class UGunnerSeatComponent;
class USpringArmComponent;
class UCameraComponent;
class UVehicleAccessPoint;
class ABaseUnit;

UCLASS()
class ADBELLUM_API ABaseVehicle : public AAVS_Vehicle, public IEnterable, public IFormable, public ISelectable, public IOrderable, public ICustomizable, public IVehicle, public IALSInputInterface, public IALSADSInterface, public ISelectionMenuProvider
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

	// Every seat sorted by component name: the same order on every machine (selection menu)
	TArray<UVehicleSeatComponent*> GetSortedSeats() const;

	// First access point that lists Seat, the point a unit in that seat leaves through
	UVehicleAccessPoint* FindAccessPointForSeat(const UVehicleSeatComponent* Seat) const;

	// True when a player sits in Seat (directly, or drives the vehicle from it)
	bool IsSeatHeldByPlayer(const UVehicleSeatComponent* Seat) const;

	// Server: puts Unit in Seat. Entering through an access point and switching seats both use it.
	// bSwitchingSeat - the unit already sits in this vehicle: moved to the new seat at once, no sit down.
	bool OccupySeat(AALSBaseCharacter* Unit, UVehicleSeatComponent* Seat, UVehicleAccessPoint* ExitPoint, bool bSwitchingSeat);

	// Server: Unit (a player's unit) moves to TargetSeat. A free seat is taken, an AI unit in it swaps
	// to Unit's old seat, a seat held by another player is refused.
	bool ServerSwitchSeat(ABaseUnit* Unit, UVehicleSeatComponent* TargetSeat);

	// ISelectionMenuProvider: the vehicle's seats
	virtual void GetSelectionMenuOptions_Implementation(APlayerController* Player, TArray<FSelectionMenuOption>& Options, int32& CurrentIndex) override;
	virtual void HandleSelectionMenuChoice_Implementation(APlayerController* Player, int32 Index) override;

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

	// Handbrake, on while the key is held
	virtual void JumpAction_Implementation(bool bValue) override;

	virtual void InteractionAction_Implementation() override;

	UFUNCTION(Server, Reliable)
	void Server_InteractionAction();

	// DRIVER CAMERA
	// One look direction, relative to the vehicle (turns with it), drives the first person view, the third
	// person arm and the driver's head. The driver's machine owns it and sends it to the server for the others.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> ThirdPersonCameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	// First person at the driver's head. Off - third person (debugging), toggled with the camera held action.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driver Camera")
	bool bDriverFirstPerson = true;

	// Socket of the driver unit's mesh the first person camera sits at
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	FName FirstPersonCameraSocket = TEXT("FP_Camera");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	float FirstPersonFOV = 90.0f;

	// Look limits relative to the vehicle (degrees, X - min, Y - max)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	FVector2D FirstPersonYawLimit = FVector2D(-135.0f, 135.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	FVector2D FirstPersonPitchLimit = FVector2D(-60.0f, 45.0f);

	// Third person turns all the way around, only pitch is limited
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	FVector2D ThirdPersonPitchLimit = FVector2D(-45.0f, 20.0f);

	// Degrees per unit of look input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	float DriverLookRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver Camera")
	bool bInvertDriverLookPitch = false;

	// Driver look relative to the vehicle: exact on the driver's machine, replicated everywhere else
	UFUNCTION(BlueprintPure, Category = "Driver Camera")
	FRotator GetDriverLookRotation() const;

	UFUNCTION(BlueprintPure, Category = "Driver Camera")
	FRotator GetDriverLookWorldRotation() const;

	virtual void CameraUpAction_Implementation(float Value) override;

	virtual void CameraRightAction_Implementation(float Value) override;

	// Toggles first / third person
	virtual void CameraHeldAction_Implementation() override;

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	virtual void NotifyControllerChanged() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Server, Unreliable)
	void Server_SetDriverLookRotation(FRotator NewLook);

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

	// Server: takes Unit out of its seat without standing it up (seat switch)
	void VacateSeatForSwitch(AALSBaseCharacter* Unit);

	// The unit Player plays in this vehicle, null when it is not in it
	ABaseUnit* GetPlayerUnit(const APlayerController* Player) const;

	// DRIVER CAMERA
	UPROPERTY(Replicated)
	FRotator ReplicatedDriverLook = FRotator::ZeroRotator;

	// Look on the driver's machine, ahead of ReplicatedDriverLook by the network delay
	FRotator LocalDriverLook = FRotator::ZeroRotator;

	FTimerHandle DriverLookSendTimer;

	void AddDriverLook(float YawInput, float PitchInput);
	FRotator ClampDriverLook(FRotator Look) const;
	void SendDriverLook();

	// Hides the head of the driver unit on this machine while it looks through it (first person)
	void UpdateDriverHeadVisibility(bool bHide);

	TWeakObjectPtr<AALSBaseCharacter> HiddenHeadUnit;
};
