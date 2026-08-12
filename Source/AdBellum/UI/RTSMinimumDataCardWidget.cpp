#include "RTSMinimumDataCardWidget.h"
#include "Unit/BaseUnit.h"
#include "Formation/BaseFormation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"

URTSMinimumDataCardWidget::URTSMinimumDataCardWidget(const FObjectInitializer& ObjInit)
    : Super(ObjInit)
{
}

void URTSMinimumDataCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // If Begin Play - create children if they do not exist
    if (!RootButton)
    {
        RootButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RootButton"));
        BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
        IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
        PrimaryTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimaryText"));
        SecondaryTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SecondaryText"));
        StatusTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));

        UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
        UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox"));

        WidgetTree->RootWidget = RootButton;
        RootButton->AddChild(BackgroundBorder);
        BackgroundBorder->SetPadding(FMargin(4));
        BackgroundBorder->SetContent(H);

        H->AddChildToHorizontalBox(IconImage);
        H->AddChildToHorizontalBox(V);

        V->AddChildToVerticalBox(PrimaryTextBlock);
        V->AddChildToVerticalBox(SecondaryTextBlock);
        V->AddChildToVerticalBox(StatusTextBlock);

        RootButton->OnClicked.AddDynamic(this, &URTSMinimumDataCardWidget::NotifyClicked);
    }
}

void URTSMinimumDataCardWidget::BindToFormation(ABaseFormation* Formation)
{
    BoundObject = Formation;
    if (!IsValid(Formation)) return;

    // Basic population: use actor name. More detailed population can be done by calling SetData from caller.
    if (PrimaryTextBlock)
        PrimaryTextBlock->SetText(FText::FromString(Formation->GetName()));
    if (SecondaryTextBlock)
        SecondaryTextBlock->SetText(FText::GetEmpty());
    if (StatusTextBlock)
        StatusTextBlock->SetText(FText::GetEmpty());
}

void URTSMinimumDataCardWidget::BindToUnit(ABaseUnit* Unit)
{
    BoundObject = Unit;
    if (!IsValid(Unit)) return;

    if (PrimaryTextBlock)
        PrimaryTextBlock->SetText(FText::FromString(Unit->GetName()));
    if (SecondaryTextBlock)
        SecondaryTextBlock->SetText(FText::GetEmpty());
    if (StatusTextBlock)
        StatusTextBlock->SetText(FText::GetEmpty());
}

void URTSMinimumDataCardWidget::SetData(UTexture2D* Icon, const FString& Primary, const FString& Secondary, const FString& Status)
{
    if (IconImage && Icon)
        IconImage->SetBrushFromTexture(Icon);
    if (PrimaryTextBlock)
        PrimaryTextBlock->SetText(FText::FromString(Primary));
    if (SecondaryTextBlock)
        SecondaryTextBlock->SetText(FText::FromString(Secondary));
    if (StatusTextBlock)
        StatusTextBlock->SetText(FText::FromString(Status));
}

void URTSMinimumDataCardWidget::SetSelected(bool bSelected)
{
    if (BackgroundBorder)
    {
        BackgroundBorder->SetBrushColor(bSelected ? FLinearColor(0.2f, 0.6f, 1.0f, 0.25f) : FLinearColor::White);
    }
}

void URTSMinimumDataCardWidget::NotifyClicked()
{
    if (OnCardClicked.IsBound())
    {
        OnCardClicked.Broadcast(BoundObject.Get());
    }
}
