// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "TelekinesisAIPerceptionComponent.generated.h"

/**
 * Extended AI Perception component that can detect telekinesis usage
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class LIANE_API UTelekinesisAIPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UTelekinesisAIPerceptionComponent();

	// Telekinesis detection settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Telekinesis Perception")
	float TelekinesisDetectionRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Telekinesis Perception")
	float TelekinesisDetectionInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Telekinesis Perception")
	bool bDetectObjectGrab = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Telekinesis Perception")
	bool bDetectObjectThrow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Telekinesis Perception")
	bool bDetectHealthDrain = true;

	// Telekinesis detection functions
	UFUNCTION(BlueprintCallable, Category = "AI|Telekinesis Perception")
	void OnTelekinesisDetected(AActor* TelekinesisUser, AActor* AffectedObject, FVector Location, FName TelekinesisAction);

	UFUNCTION(BlueprintCallable, Category = "AI|Telekinesis Perception")
	bool IsTelekinesisDetected() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Telekinesis Perception")
	AActor* GetLastTelekinesisUser() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Telekinesis Perception")
	FVector GetLastTelekinesisLocation() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Telekinesis Perception")
	FName GetLastTelekinesisAction() const;

	// Blueprint event for telekinesis detection
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|Telekinesis Perception")
	void OnTelekinesisDetectedEvent(AActor* TelekinesisUser, AActor* AffectedObject, FVector Location, FName TelekinesisAction);

protected:
	// Override BeginPlay to set up telekinesis detection
	virtual void BeginPlay() override;

	// Override TickComponent to handle telekinesis detection
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Telekinesis detection state
	bool bIsTelekinesisDetected;
	AActor* LastTelekinesisUser;
	AActor* LastAffectedObject;
	FVector LastTelekinesisLocation;
	FName LastTelekinesisAction;
	float TimeSinceLastDetection;

	// Helper function to update blackboard with telekinesis information
	void UpdateBlackboardWithTelekinesisInfo();
};