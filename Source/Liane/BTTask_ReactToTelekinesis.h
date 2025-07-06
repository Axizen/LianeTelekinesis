// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReactToTelekinesis.generated.h"

// Reaction type enum
UENUM(BlueprintType)
enum class ETelekinesisReactionType : uint8
{
	Flee UMETA(DisplayName = "Flee"),
	TakeCover UMETA(DisplayName = "Take Cover"),
	Attack UMETA(DisplayName = "Attack"),
	CallForHelp UMETA(DisplayName = "Call For Help"),
	Investigate UMETA(DisplayName = "Investigate")
};

/**
 * Behavior Tree Task that makes AI react to telekinesis
 */
UCLASS()
class LIANE_API UBTTask_ReactToTelekinesis : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ReactToTelekinesis();

	// Blackboard key selectors
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TelekinesisUserKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TelekinesisLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector FleeLocationKey;

	// Reaction settings
	UPROPERTY(EditAnywhere, Category = "Reaction")
	ETelekinesisReactionType ReactionType;

	UPROPERTY(EditAnywhere, Category = "Reaction", meta = (EditCondition = "ReactionType == ETelekinesisReactionType::Flee"))
	float FleeDistance = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Reaction", meta = (EditCondition = "ReactionType == ETelekinesisReactionType::TakeCover"))
	float MinCoverDistance = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Reaction", meta = (EditCondition = "ReactionType == ETelekinesisReactionType::TakeCover"))
	float MaxCoverDistance = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Reaction")
	float ReactionDuration = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Reaction")
	bool bSetFocus = true;

	// Override ExecuteTask to implement the task logic
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Override GetStaticDescription to provide a description in the editor
	virtual FString GetStaticDescription() const override;

protected:
	// Helper functions for different reaction types
	EBTNodeResult::Type ExecuteFlee(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser);
	EBTNodeResult::Type ExecuteTakeCover(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser);
	EBTNodeResult::Type ExecuteAttack(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser);
	EBTNodeResult::Type ExecuteCallForHelp(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, AActor* TelekinesisUser);
	EBTNodeResult::Type ExecuteInvestigate(UBehaviorTreeComponent& OwnerComp, AAIController* AIController, FVector TelekinesisLocation);

	// Helper function to find a flee location
	bool FindFleeLocation(AAIController* AIController, AActor* TelekinesisUser, FVector& OutLocation);

	// Helper function to find cover
	bool FindCoverLocation(AAIController* AIController, AActor* TelekinesisUser, FVector& OutLocation);
};