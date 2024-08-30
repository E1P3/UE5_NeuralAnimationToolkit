// Copyright Epic Games, Inc. All Rights Reserved.

#include "NeuralAnimationToolkitCommands.h"

#define LOCTEXT_NAMESPACE "FNeuralAnimationToolkitModule"

void FNeuralAnimationToolkitCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "NeuralAnimationToolkit", "Bring up NeuralAnimationToolkit window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
