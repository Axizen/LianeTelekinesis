// Copyright Epic Games, Inc. All Rights Reserved.

#include "LianeGameMode.h"
#include "LianeCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALianeGameMode::ALianeGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/BP_LianeCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
