#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimNode_NN.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_NN.generated.h"

/**
 *
 */
UCLASS()
class NEURALANIMATIONTOOLKIT_API UAnimGraphNode_NN : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_NN Node;

	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

};
