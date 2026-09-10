#include "RTSFormationCardWidget.h"
#include "Formation/BaseFormation.h"
#include "Logging/LogMacros.h"

void URTSFormationCardWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URTSFormationCardWidget::InitializeWidget(ABaseFormation* InFormation)
{
	CardButton->OnClicked.AddDynamic(this, &URTSFormationCardWidget::OnCardButtonClicked);
	Formation = InFormation;
	// get ui data from formation, set up button image and text

}

void URTSFormationCardWidget::Clear()
{

}

void URTSFormationCardWidget::OnCardButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnCardButtonClicked: Formation=%s, Index=%d, DelegateBound=%d"),
		Formation ? *Formation->GetName() : TEXT("nullptr"), Index, OnFormationCardClicked.IsBound());
	OnFormationCardClicked.ExecuteIfBound(Formation, Index);
}
