// Fill out your copyright notice in the Description page of Project Settings.

#include "TelekinesisAIPerceptionComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

UTelekinesisAIPerceptionComponent::UTelekinesisAIPerceptionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize telekinesis detection state
	bIsTelekinesisDetected = false;
	LastTelekinesisUser = nullptr;
	LastAffectedObject = nullptr;
	LastTelekinesisLocation = FVector::ZeroVector;
	LastTelekinesisAction = NAME_None;
	TimeSinceLastDetection = 0.0f;
}

void UTelekinesisAIPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Additional setup if needed
}

void UTelekinesisAIPerceptionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update telekinesis detection timer
	if (bIsTelekinesisDetected)
	{
		TimeSinceLastDetection += DeltaTime;

		// Reset detection after interval
		if (TimeSinceLastDetection > TelekinesisDetectionInterval)
		{
			bIsTelekinesisDetected = false;
			UpdateBlackboardWithTelekinesisInfo();
		}
	}
}

void UTelekinesisAIPerceptionComponent::OnTelekinesisDetected(AActor* TelekinesisUser, AActor* AffectedObject, FVector Location, FName TelekinesisAction)
{
	// Update telekinesis detection state
	bIsTelekinesisDetected = true;
	LastTelekinesisUser = TelekinesisUser;
	LastAffectedObject = AffectedObject;
	LastTelekinesisLocation = Location;
	LastTelekinesisAction = TelekinesisAction;
	TimeSinceLastDetection = 0.0f;

	// Update blackboard
	UpdateBlackboardWithTelekinesisInfo();

	// Call blueprint event
	OnTelekinesisDetectedEvent(TelekinesisUser, AffectedObject, Location, TelekinesisAction);
}

bool UTelekinesisAIPerceptionComponent::IsTelekinesisDetected() const
{
	return bIsTelekinesisDetected;
}

AActor* UTelekinesisAIPerceptionComponent::GetLastTelekinesisUser() const
{
	return LastTelekinesisUser;
}

FVector UTelekinesisAIPerceptionComponent::GetLastTelekinesisLocation() const
{
	return LastTelekinesisLocation;
}

FName UTelekinesisAIPerceptionComponent::GetLastTelekinesisAction() const
{
	return LastTelekinesisAction;
}

void UTelekinesisAIPerceptionComponent::UpdateBlackboardWithTelekinesisInfo()
{
	// Get the AI controller
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController)
	{
		return;
	}

	// Get the blackboard
	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	// Update blackboard keys
	// Note: These keys need to be defined in the blackboard asset
	
	// Update telekinesis detected flag
	if (BlackboardComp->GetKeyID("TelekinesisDetected") != FBlackboard::InvalidKey)
	{
		BlackboardComp->SetValueAsBool("TelekinesisDetected", bIsTelekinesisDetected);
	}

	// Update telekinesis user
	if (BlackboardComp->GetKeyID("TelekinesisUser") != FBlackboard::InvalidKey)
	{
		BlackboardComp->SetValueAsObject("TelekinesisUser", LastTelekinesisUser);
	}

	// Update telekinesis location
	if (BlackboardComp->GetKeyID("TelekinesisLocation") != FBlackboard::InvalidKey)
	{
		BlackboardComp->SetValueAsVector("TelekinesisLocation", LastTelekinesisLocation);
	}

	// Update telekinesis action name
	if (BlackboardComp->GetKeyID("TelekinesisAction") != FBlackboard::InvalidKey)
	{
		BlackboardComp->SetValueAsName("TelekinesisAction", LastTelekinesisAction);
	}

	// Update affected object
	if (BlackboardComp->GetKeyID("TelekinesisAffectedObject") != FBlackboard::InvalidKey)
	{
		BlackboardComp->SetValueAsObject("TelekinesisAffectedObject", LastAffectedObject);
	}

	// Update being manipulated flag (if this AI is the affected object)
	if (BlackboardComp->GetKeyID("IsBeingTelekineticallyManipulated") != FBlackboard::InvalidKey)
	{
		bool bIsBeingManipulated = (LastAffectedObject == GetOwner()->GetInstigator());
		BlackboardComp->SetValueAsBool("IsBeingTelekineticallyManipulated", bIsBeingManipulated);
	}
}