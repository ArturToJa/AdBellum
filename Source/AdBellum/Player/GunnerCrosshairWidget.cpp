// Fill out your copyright notice in the Description page of Project Settings.


#include "GunnerCrosshairWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Unit/BaseUnit.h"
#include "Vehicle/BaseVehicle.h"
#include "Vehicle/GunnerSeatComponent.h"
#include "Weapon/IWeapon.h"

UGunnerCrosshairWidget::UGunnerCrosshairWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultCrosshairFinder(
		TEXT("/Game/DVP/Shared_Vehicle_Assets/UI/T_Crosshair_7.T_Crosshair_7"));
	if (DefaultCrosshairFinder.Succeeded())
	{
		CrosshairTexture = DefaultCrosshairFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> BodyFinder(
		TEXT("/Game/DVP/Shared_Vehicle_Assets/UI/T_APC_body_HUD_icon.T_APC_body_HUD_icon"));
	if (BodyFinder.Succeeded())
	{
		BodyTexture = BodyFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> ArmorLeftFinder(
		TEXT("/Game/DVP/Shared_Vehicle_Assets/UI/T_APC_Armor_Left_HUD_icon.T_APC_Armor_Left_HUD_icon"));
	if (ArmorLeftFinder.Succeeded())
	{
		ArmorLeftTexture = ArmorLeftFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> ArmorRightFinder(
		TEXT("/Game/DVP/Shared_Vehicle_Assets/UI/T_APC_Armor_Right_HUD_icon.T_APC_Armor_Right_HUD_icon"));
	if (ArmorRightFinder.Succeeded())
	{
		ArmorRightTexture = ArmorRightFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> GunLineFinder(
		TEXT("/Game/DVP/Shared_Vehicle_Assets/UI/T__gun_HUD_line.T__gun_HUD_line"));
	if (GunLineFinder.Succeeded())
	{
		GunLineTexture = GunLineFinder.Object;
	}
}

UImage* UGunnerCrosshairWidget::AddTurretIcon(UCanvasPanel* Root, FName Name, UTexture2D* Texture, FVector2D LocalOffset) const
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
	UCanvasPanelSlot* IconSlot = Root->AddChildToCanvas(Image);
	// Anchored at 30% of screen height above the bottom edge (percentage, not pixels, so it stays correctly
	// placed at any resolution) rather than relative to the crosshair. LocalOffset is this icon's own small
	// pixel offset within the group - same relative layout as the old BP_Gunner_Widget's TurretRotation canvas
	// (Body/Armor_left/Armor_right/Main_gun_line).
	IconSlot->SetAnchors(FAnchors(0.5f, 0.7f));
	IconSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	IconSlot->SetAutoSize(false);
	IconSlot->SetSize(TurretRotationIconSize);
	IconSlot->SetPosition(LocalOffset);

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = TurretRotationIconSize;
	Brush.TintColor = FSlateColor(TurretRotationColor);
	Image->SetBrush(Brush);
	Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	return Image;
}

TSharedRef<SWidget> UGunnerCrosshairWidget::RebuildWidget()
{
	// Always rebuild from the current property values rather than reusing a tree built once: a Blueprint
	// subclass's first-ever build gets baked into its CDO, so a one-time guard here would permanently lock
	// every instance (including ones made after later editing CrosshairSize/Offset/etc.) to those stale values.
	WidgetTree->RootWidget = nullptr;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrosshairRoot"));
	WidgetTree->RootWidget = Root;

	CrosshairImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CrosshairImage"));
	CrosshairImageSlot = Root->AddChildToCanvas(CrosshairImage);
	CrosshairImageSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	CrosshairImageSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CrosshairImageSlot->SetAutoSize(false);
	CrosshairImageSlot->SetSize(CrosshairSize);
	CrosshairImageSlot->SetPosition(CrosshairOffset);

	FSlateBrush Brush;
	Brush.SetResourceObject(CrosshairTexture);
	Brush.ImageSize = CrosshairSize;
	Brush.TintColor = FSlateColor(CrosshairColor);
	CrosshairImage->SetBrush(Brush);
	CrosshairImage->SetVisibility(StationaryRole == EALSStationaryRole::Gunner ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	// Turret-relative-to-hull rotation indicator, below the crosshair: vehicle body + armor pips (static)
	// plus the rotating gun line on top, same relative offsets as the old TurretRotation canvas
	BodyImage = AddTurretIcon(Root, TEXT("BodyImage"), BodyTexture, FVector2D(6.0f, 0.0f));
	ArmorLeftImage = AddTurretIcon(Root, TEXT("ArmorLeftImage"), ArmorLeftTexture, FVector2D(-18.0f, 12.0f));
	ArmorRightImage = AddTurretIcon(Root, TEXT("ArmorRightImage"), ArmorRightTexture, FVector2D(30.0f, 12.0f));
	GunLineImage = AddTurretIcon(Root, TEXT("GunLineImage"), GunLineTexture, FVector2D(6.0f, -8.0f));

	return Super::RebuildWidget();
}

void UGunnerCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);
	UpdateCrosshairTexture();
	UpdateCrosshairOffset();
	UpdateTurretRotationIndicator();
}

const UGunnerSeatComponent* UGunnerCrosshairWidget::GetGunnerSeat() const
{
	const ABaseUnit* Unit = Cast<ABaseUnit>(GetOwningPlayerPawn());
	return Unit ? Cast<UGunnerSeatComponent>(Unit->OccupiedSeat) : nullptr;
}

void UGunnerCrosshairWidget::SetStationaryRole(EALSStationaryRole Role)
{
	StationaryRole = Role;
	if (CrosshairImage)
	{
		// Gunner_Standing aims straight through its weapon's own sight, which draws its own reticle -
		// this crosshair is only for an enclosed gunner looking through the vehicle's MCT socket.
		CrosshairImage->SetVisibility(Role == EALSStationaryRole::Gunner ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGunnerCrosshairWidget::UpdateCrosshairTexture()
{
	if (!CrosshairImage)
	{
		return;
	}

	const ABaseUnit* Unit = Cast<ABaseUnit>(GetOwningPlayerPawn());
	AActor* ActiveWeapon = Unit ? Unit->ActiveWeaponActor : nullptr;
	UTexture2D* Reticle = ActiveWeapon ? IIWeapon::Execute_GetWeaponHUDReticle(ActiveWeapon) : nullptr;
	CrosshairImage->SetBrushResourceObject(Reticle ? Reticle : CrosshairTexture.Get());

	// A weapon can ask for its own on screen reticle size (e.g. a larger ATGM reticle vs. a tight MG one) -
	// zero means no override, keep this widget's own default CrosshairSize.
	const FVector2D ReticleSize = ActiveWeapon ? IIWeapon::Execute_GetWeaponReticleSize(ActiveWeapon) : FVector2D::ZeroVector;
	const FVector2D Size = ReticleSize.IsNearlyZero() ? CrosshairSize : ReticleSize;
	CrosshairImage->SetDesiredSizeOverride(Size);
	if (CrosshairImageSlot)
	{
		CrosshairImageSlot->SetSize(Size);
	}
}

void UGunnerCrosshairWidget::UpdateCrosshairOffset()
{
	if (!CrosshairImageSlot)
	{
		return;
	}
	const UGunnerSeatComponent* GunnerSeat = GetGunnerSeat();
	CrosshairImageSlot->SetPosition(GunnerSeat ? GunnerSeat->CrosshairOffset : CrosshairOffset);
}

void UGunnerCrosshairWidget::UpdateTurretRotationIndicator()
{
	if (!GunLineImage)
	{
		return;
	}

	const UGunnerSeatComponent* GunnerSeat = GetGunnerSeat();
	const ABaseVehicle* Vehicle = GunnerSeat ? Cast<ABaseVehicle>(GunnerSeat->GetOwner()) : nullptr;
	const USkeletalMeshComponent* Mesh = Vehicle ? Vehicle->VehicleSkeletalMesh.Get() : nullptr;
	if (!Mesh || GunnerSeat->TurretBoneName.IsNone() || !Mesh->DoesSocketExist(GunnerSeat->TurretBoneName))
	{
		GunLineImage->SetRenderTransformAngle(0.0f);
		return;
	}

	// Turret rotation relative to its parent bone (the hull), same source the old BP_Gunner_Widget's
	// Main_gun_line used (GetSocketTransform in RTS_ParentBoneSpace)
	const FRotator RelativeRotation = Mesh->GetSocketTransform(GunnerSeat->TurretBoneName, RTS_ParentBoneSpace).Rotator();
	GunLineImage->SetRenderTransformAngle(RelativeRotation.Yaw);
}
