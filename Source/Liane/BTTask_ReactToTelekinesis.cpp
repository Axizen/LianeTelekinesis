// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_ReactToTelekinesis.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTTask_ReactToTelekinesis::UBTTask_ReactToTelekinesis()
{
	// Set default values
	NodeName = TEXT("React To Telekinesis");
	ReactionType = ETelekinesisReactionType::Flee;
}

EBTNodeResult::Type UBTTask_ReactToTelekinesis::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Get the AI controller
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	// Get the blackboard
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Get telekinesis user from blackboard
	AActor* TelekinesisUser = nullptr;
	if (TelekinesisUserKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		TelekinesisUser = Cast<AActor>(BlackboardComp->GetValueAsObject(TelekinesisUserKey.SelectedKeyName));
	}

	// Get telekinesis location from blackboard
	FVector TelekinesisLocation = FVector::ZeroVector;
	if (TelekinesisLocationKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		TelekinesisLocation = BlackboardComp->GetValueAsVector(TelekinesisLocationKey.SelectedKeyName);
	}

	// Execute the appropriate reaction based on the reaction type
	switch (ReactionType)
	{
	case ETelekinesisReactionType::Flee:
		return ExecuteFlee(OwnerComp, AIController, TelekinesisUser);

	case ETelekinesisReactionType::TakeCover:
		return ExecuteTakeCover(OwnerComp, AIController, TelekinesisUser);

	case ETelekinesisReactionType::Attack:
		return ExecuteAttack(OwnerComp, AIController, TelekinesisUser);

	case ETelekinesisReactionType::CallForHelp:
		return ExecuteCallForHelp(OwnerComp, AIController, TelekinesisUser);

	case ETelekinesisReactionType::Investigate:
		return ExecuteInvestigate(OwnerComp, AIController, TelekinesisLocation);

	default:
		return EBTNodeResult::Failed;
	}
}

FString UBTTask_ReactToTelekinesis::GetStaticDescription() const
{
	// Create a description based on the reaction type
	FString TypeDesc;
	switch (ReactionType)
	{
	case ETelekinesisReactionType::Flee:
		TypeDesc = FString::Printf(TEXT("Flee from telekinesis user (%.1f units)"), FleeDistance);
		break;

	case ETelekinesisReactionType::TakeCover:
		TypeDesc = FString::Printf(TEXT("Take cover from telekinesis user (%.1f-%.1f units)"), MinCoverDistance, MaxCoverDistance);
		break;

	case ETelekinesisReactionType::Attack:
		TypeDesc = TEXT("Attack telekinesis user");
		break;

	case ETelekinesisReactionType::CallForHelp:
		TypeDesc = TEXT("Call for help against telekinesis user");
		break;

	case ETelekinesisReactionType::Investigate:
		TypeDesc = TEXT("Investigate telekinesis location");
		break;

	default:
		TypeDesc = TEXT("Unknown reaction");
		break;
	}

	return FString::Printf(TEXT("React To Telekinesis: %s"), *TypeDesc);
}

EBTNodeResult::Type UBTTask_ReactToTelekinesis::ExecuteFlee(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser)
{
	// Check if we have a valid telekinesis user
	if (!TelekinesisUser)
	{
		return EBTNodeResult::Failed;
	}

	// Get the blackboard
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Find a flee location
	FVector FleeLocation;
	if (!FindFleeLocation(AIController, TelekinesisUser, FleeLocation))
	{
		return EBTNodeResult::Failed;
	}

	// Set the flee location in the blackboard
	if (FleeLocationKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		BlackboardComp->SetValueAsVector(FleeLocationKey.SelectedKeyName, FleeLocation);
	}

	// Set focus on telekinesis user if requested
	if (bSetFocus)
	{
		AIController->SetFocus(TelekinesisUser);
	}

	// Increase movement speed for fleeing
	APawn* ControlledPawn = AIController->GetPawn();
	if (ControlledPawn)
	{
		ACharacter* Character = Cast<ACharacter>(ControlledPawn);
		if (Character && Character->GetCharacterMovement())
		{
			// Temporarily increase movement speed
			Character->GetCharacterMovement()->MaxWalkSpeed *= 1.5f;
		}
	}

	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_ReactToTelekinesis::ExecuteTakeCover(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser)
{
	// Check if we have a valid telekinesis user
	if (!TelekinesisUser)
	{
		return EBTNodeResult::Failed;
	}

	// Get the blackboard
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Find a cover location
	FVector CoverLocation;
	if (!FindCoverLocation(AIController, TelekinesisUser, CoverLocation))
	{
		// If we can't find cover, try to flee instead
		return ExecuteFlee(OwnerComp, AIController, TelekinesisUser);
	}

	// Set the cover location in the blackboard
	if (FleeLocationKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		BlackboardComp->SetValueAsVector(FleeLocationKey.SelectedKeyName, CoverLocation);
	}

	// Set focus on telekinesis user if requested
	if (bSetFocus)
	{
		AIController->SetFocus(TelekinesisUser);
	}

	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_ReactToTelekinesis::ExecuteAttack(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser)
{
	// Check if we have a valid telekinesis user
	if (!TelekinesisUser)
	{
		return EBTNodeResult::Failed;
	}

	// Set focus on telekinesis user
	AIController->SetFocus(TelekinesisUser);

	// Move to telekinesis user
	AIController->MoveToActor(TelekinesisUser, -1.0f, true, true, true, nullptr, true);

	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_ReactToTelekinesis::ExecuteCallForHelp(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser)
{
	// This would typically involve alerting nearby AI
	// For now, we'll just set focus on the telekinesis user
	if (TelekinesisUser && bSetFocus)
	{
		AIController->SetFocus(TelekinesisUser);
	}

	// In a full implementation, we would alert nearby AI here
	// This could involve broadcasting an event or using perception stimuli

	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_ReactToTelekinesis::ExecuteInvestigate(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, FVector TelekinesisLocation)
{
	// Check if we have a valid telekinesis location
	if (TelekinesisLocation.IsZero())
	{
		return EBTNodeResult::Failed;
	}

	// Get the blackboard
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Set the investigation location in the blackboard
	if (FleeLocationKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		BlackboardComp->SetValueAsVector(FleeLocationKey.SelectedKeyName, TelekinesisLocation);
	}

	// Clear focus
	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	return EBTNodeResult::Succeeded;
}

bool UBTTask_ReactToTelekinesis::FindFleeLocation(AAIController* AIController, AActor* TelekinesisUser, FVector& OutLocation)
{
	if (!AIController || !TelekinesisUser)
	{
		return false;
	}

	// Get the controlled pawn
	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	// Get the navigation system
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld());
	if (!NavSys)
	{
		return false;
	}

	// Calculate direction away from telekinesis user
	FVector FleeDirection = ControlledPawn->GetActorLocation() - TelekinesisUser->GetActorLocation();
	FleeDirection.Normalize();

	// Calculate target flee location
	FVector TargetLocation = ControlledPawn->GetActorLocation() + (FleeDirection * FleeDistance);

	// Find a valid navigation point
	FNavLocation NavLocation;
	if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
	{
		OutLocation = NavLocation.Location;
		return true;
	}

	return false;
}

bool UBTTask_ReactToTelekinesis::FindCoverLocation(AAIController* AIController, AActor* TelekinesisUser, FVector& OutLocation)
{
	if (!AIController || !TelekinesisUser)
	{
		return false;
	}

	// Get the controlled pawn
	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	// Get the navigation system
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld());
	if (!NavSys)
	{
		return false;
	}

	// In a full implementation, we would use environment queries or raycasts to find actual cover
	// For now, we'll just find a point at a certain distance

	// Calculate direction away from telekinesis user
	FVector FleeDirection = ControlledPawn->GetActorLocation() - TelekinesisUser->GetActorLocation();
	FleeDirection.Normalize();

	// Calculate target cover location
	float CoverDistance = FMath::RandRange(MinCoverDistance, MaxCoverDistance);
	FVector TargetLocation = ControlledPawn->GetActorLocation() + (FleeDirection * CoverDistance);

	// Find a valid navigation point
	FNavLocation NavLocation;
	if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
	{
		OutLocation = NavLocation.Location;
		return true;
	}

	return false;
}