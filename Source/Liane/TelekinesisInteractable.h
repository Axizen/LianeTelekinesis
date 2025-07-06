// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TelekinesisInteractable.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTelekinesisInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that can be manipulated with telekinesis
 */
class LIANE_API ITelekinesisInteractable
{
	GENERATED_BODY()

public:
	// Check if this object can be grabbed with telekinesis
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	bool CanBeGrabbed() const;

	// Called when the object is grabbed with telekinesis
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void OnGrabbed(AActor* GrabbedBy);

	// Called when the object is released from telekinesis
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void OnReleased(AActor* ReleasedBy, bool bThrowPerformed);

	// Called when the object is thrown with telekinesis
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void OnThrown(AActor* ThrownBy, FVector ThrowForce);

	// Get the mass of the object for telekinesis calculations
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	float GetTelekinesisMass() const;

	// Apply a telekinetic force to the object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void ApplyTelekineticForce(FVector Force, FName BoneName);

	// Check if this object can orbit around another object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	bool CanOrbit() const;

	// Called when the object starts orbiting
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void OnOrbitStart(AActor* OrbitAround);

	// Called when the object stops orbiting
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void OnOrbitEnd();

	// Called when the object impacts something while under telekinetic control
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void OnTelekineticImpact(AActor* HitActor, FVector ImpactPoint, FVector ImpactNormal, float ImpactForce);
};