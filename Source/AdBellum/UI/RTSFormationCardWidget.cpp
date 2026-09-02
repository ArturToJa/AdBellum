#include "RTSFormationCardWidget.h"
#include "Formation/BaseFormation.h"

void URTSFormationCardWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URTSFormationCardWidget::InitializeWidget(ABaseFormation* InFormation)
{
	CardButton->OnClicked.AddDynamic(this, &URTSFormationCardWidget::OnCardButtonClicked);
	Formation = InFormation;

}

void URTSFormationCardWidget::OnCardButtonClicked()
{
	OnFormationCardClicked.ExecuteIfBound(Formation, Index);
}
