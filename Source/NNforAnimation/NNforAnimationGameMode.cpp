// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNforAnimationGameMode.h"
#include "NNforAnimationCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANNforAnimationGameMode::ANNforAnimationGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
