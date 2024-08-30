#include "AnimGraphNode_NN.h"
#include "AnimationGraphSchema.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_NN"

FLinearColor UAnimGraphNode_NN::GetNodeTitleColor() const
{
	return FColor(86, 182, 194);
}

FText UAnimGraphNode_NN::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Run Neural Net");
}

FText UAnimGraphNode_NN::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Neural Network");
}

FText UAnimGraphNode_NN::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Neural Network");
}

#undef LOCTEXT_NAMESPACE