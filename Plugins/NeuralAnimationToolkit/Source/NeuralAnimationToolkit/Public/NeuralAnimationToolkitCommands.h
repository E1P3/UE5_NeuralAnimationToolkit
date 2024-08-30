// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "NeuralAnimationToolkitStyle.h"

class FNeuralAnimationToolkitCommands : public TCommands<FNeuralAnimationToolkitCommands>
{
public:

	FNeuralAnimationToolkitCommands()
		: TCommands<FNeuralAnimationToolkitCommands>(TEXT("NeuralAnimationToolkit"), NSLOCTEXT("Contexts", "NeuralAnimationToolkit", "NeuralAnimationToolkit Plugin"), NAME_None, FNeuralAnimationToolkitStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};