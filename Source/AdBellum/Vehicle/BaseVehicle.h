// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enterable.h"
#include "OrderSystem/Orderable.h"
#include "Interfaces/Customizable.h"
#include "Vehicle/Vehicle.h"
#include "Interfaces/Selectable.h"
#include "VehicleSystemBase.h"
#include "VehicleSeatComponent.h"
#include "Library/ALSADSInterface.h"
#include "Weapon/BaseWeapon.h"
#include "Weapon/WeaponChildComponent.h"
#include "BaseVehicle.generated.h"

class UNetworkComponent;
class ABaseWeapon;

UCLASS()
class ADBELLUM_API ABaseVehicle : public AVehicleSystemBase, public IEnterable, public IFormable, public ISelectable, public IOrderable, public ICustomizable, public IVehicle, public IALSInputInterface, public IALSADSInterface
{
	GENERATED_BODY()
public:

	ABaseVehicle(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNetworkComponent> NetworkComponent;

	//possession

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	AUnitAIController* BackupAIController = nullptr;

	void PossessByAIController();

	UVehicleSeatComponent* DriverSeat;


	// DAMAGE 
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	float HP = 5000.0f;

	UFUNCTION()
	void OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUpdateHealth(float Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle - General")
	TObjectPtr<USkeletalMeshComponent> VehicleSkeletalMesh;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float TurretYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float TurretTurnRate = 0.66f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float GunPitch = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float GunPitchRate = 0.35f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* GunnerADS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Setup")
	FName ADSSocketName = "ADS_Gunner";

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

	//Input interface

	virtual void GunnerCameraRightAction_Implementation(float Value) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_GunnerCameraRightAction(float Value);

	virtual void GunnerCameraUpAction_Implementation(float Value) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_GunnerCameraUpAction(float Value);

	virtual void InteractionAction_Implementation() override;

	UFUNCTION(Server, Reliable)
	void Server_InteractionAction();

	//virtual void PrimarySelectionAction_Implementation() override;
	//virtual void SecondarySelectionAction_Implementation() override;
	//virtual void ThirdSelectionAction_Implementation() override;

	//Vehicle interaface
	virtual void CalibrateADS_Implementation(FVector TargetVector) override;

	//ADS Interface
	virtual FVector getADSTarget_Implementation() override;

	virtual FRotator getADSRotation_Implementation() override;

	virtual FTransform getCameraTransform_Implementation(EALSStationaryRole StationaryRole) override;

	void CalibrateGunnerADS();

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
};
