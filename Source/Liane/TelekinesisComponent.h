// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TelekinesisComponent.generated.h"

// Forward declarations
class UPhysicsHandleComponent;
class UHealthComponent;

// Event delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectGrabbed, AActor*, GrabbedObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectReleased, AActor*, ReleasedObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectThrown, AActor*, ThrownObject, FVector, ThrowForce);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectImpact, AActor*, ImpactedObject, FVector, ImpactPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDrainStart, AActor*, TargetActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDrainStop, AActor*, TargetActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrbitalSystemActivated, AActor*, CenterObject);

/**
 * TelekinesisComponent provides physics-based telekinesis abilities
 * Allows grabbing, moving, rotating, and throwing physics objects
 * Supports orbital system for smaller objects around a grabbed large object
 * Can drain health from enemies and apply physics damage
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class LIANE_API UTelekinesisComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTelekinesisComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called when the component is destroyed
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//////////////////////////////////////////////////////////////////////////
	// Core Telekinesis Functions

	/** Attempts to grab an object in front of the player */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Core")
	bool GrabObject();

	/** Releases the currently grabbed object */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Core")
	void ReleaseObject(bool bApplyVelocity = false);

	/** Moves the grabbed object to the specified location */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Core")
	void MoveObjectToLocation(FVector TargetLocation);

	/** Rotates the grabbed object by the specified rotation */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Core")
	void RotateObject(FRotator DeltaRotation);

	/** Starts charging a throw */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Core")
	void StartChargeThrow();

	/** Throws the grabbed object with the current charge */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Core")
	void ThrowObject();

	/** Checks if an object is currently grabbed */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Core")
	bool IsObjectGrabbed() const;

	/** Checks if a throw is being charged */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Core")
	bool IsCharging() const;

	/** Gets the current throw charge time */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Core")
	float GetCurrentChargeTime() const;

	/** Gets the current throw charge percentage (0.0-1.0) */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Core")
	float GetChargePercentage() const;

	/** Gets the currently grabbed object */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Core")
	AActor* GetGrabbedObject() const;

	//////////////////////////////////////////////////////////////////////////
	// Health Drain Functions

	/** Starts draining health from a target in front of the player */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Health")
	void StartHealthDrain();

	/** Stops draining health */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Health")
	void StopHealthDrain();

	/** Checks if currently draining health */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Health")
	bool IsDrainingHealth() const;

	/** Gets the current health drain target */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Health")
	AActor* GetHealthDrainTarget() const;

	//////////////////////////////////////////////////////////////////////////
	// Orbital System Functions

	/** Activates the orbital system around the grabbed object */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Orbital")
	void ActivateOrbitalSystem();

	/** Deactivates the orbital system */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Orbital")
	void DeactivateOrbitalSystem();

	/** Checks if the orbital system is active */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Orbital")
	bool IsOrbitalSystemActive() const;

	/** Gets the current orbital objects */
	UFUNCTION(BlueprintPure, Category = "Telekinesis|Orbital")
	TArray<AActor*> GetOrbitalObjects() const;

	//////////////////////////////////////////////////////////////////////////
	// Blueprint Implementable Functions

	/** Determines if an object can be grabbed with telekinesis */
	UFUNCTION(BlueprintNativeEvent, Category = "Telekinesis")
	bool CanGrabObject(UPrimitiveComponent* Component);

	/** Modifies the throw force before applying it */
	UFUNCTION(BlueprintNativeEvent, Category = "Telekinesis")
	void ModifyThrowForce(FVector& ThrowForce, float ChargeAmount, AActor* ThrownObject);

	/** Calculates damage based on impact force */
	UFUNCTION(BlueprintNativeEvent, Category = "Telekinesis")
	float CalculateDamage(AActor* HitActor, AActor* ThrownObject, float ImpactForce);

	/** Called when the telekinesis target changes */
	UFUNCTION(BlueprintNativeEvent, Category = "Telekinesis")
	void OnTelekinesisTargetChanged(AActor* NewTarget, AActor* PreviousTarget);

	//////////////////////////////////////////////////////////////////////////
	// Event Dispatchers

	/** Event fired when an object is grabbed */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnObjectGrabbed OnObjectGrabbed;

	/** Event fired when an object is released */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnObjectReleased OnObjectReleased;

	/** Event fired when an object is thrown */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnObjectThrown OnObjectThrown;

	/** Event fired when a thrown object impacts something */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnObjectImpact OnObjectImpact;

	/** Event fired when health drain starts */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnHealthDrainStart OnHealthDrainStart;

	/** Event fired when health drain stops */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnHealthDrainStop OnHealthDrainStop;

	/** Event fired when the orbital system is activated */
	UPROPERTY(BlueprintAssignable, Category = "Telekinesis|Events")
	FOnOrbitalSystemActivated OnOrbitalSystemActivated;

	//////////////////////////////////////////////////////////////////////////
	// Core Telekinesis Parameters

	/** Maximum distance at which objects can be grabbed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "100.0", ClampMax = "5000.0", UIMin = "100.0", UIMax = "5000.0"))
	float MaxGrabDistance = 1000.0f;

	/** Maximum mass of objects that can be grabbed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxGrabbableMass = 200.0f;

	/** How quickly grabbed objects move to target position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "1.0", ClampMax = "50.0", UIMin = "1.0", UIMax = "50.0"))
	float GrabInterpolationSpeed = 5.0f;

	/** How far in front of the camera to hold objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "50.0", ClampMax = "1000.0", UIMin = "50.0", UIMax = "1000.0"))
	float HoldDistance = 300.0f;

	/** Rotation speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0"))
	float RotationSpeed = 1.0f;

	/** Linear damping applied to grabbed objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "10.0"))
	float GrabbedObjectDamping = 5.0f;

	/** Angular damping applied to grabbed objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Core", meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "10.0"))
	float GrabbedObjectAngularDamping = 5.0f;

	//////////////////////////////////////////////////////////////////////////
	// Throwing Parameters

	/** Base force multiplier for thrown objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Throwing", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float ThrowForceMultiplier = 1000.0f;

	/** Maximum charge time for throws in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Throwing", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0"))
	float MaxThrowChargeTime = 2.0f;

	/** Minimum force applied when throwing without charging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Throwing", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	float MinThrowForcePercent = 0.2f;

	/** Damage multiplier for thrown objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Throwing", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ThrowDamageMultiplier = 0.1f;

	//////////////////////////////////////////////////////////////////////////
	// Orbital System Parameters

	/** Maximum number of objects that can orbit a grabbed object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Orbital", meta = (ClampMin = "0", ClampMax = "20", UIMin = "0", UIMax = "20"))
	int32 MaxOrbitalObjects = 5;

	/** Radius of the orbital field around grabbed objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Orbital", meta = (ClampMin = "50.0", ClampMax = "1000.0", UIMin = "50.0", UIMax = "1000.0"))
	float OrbitalRadius = 200.0f;

	/** Speed at which objects orbit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Orbital", meta = (ClampMin = "10.0", ClampMax = "500.0", UIMin = "10.0", UIMax = "500.0"))
	float OrbitalSpeed = 100.0f;

	/** Maximum mass of objects that can be added to the orbital system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Orbital", meta = (ClampMin = "0.1", ClampMax = "100.0", UIMin = "0.1", UIMax = "100.0"))
	float MaxOrbitalObjectMass = 20.0f;

	/** Force applied to pull objects into orbit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Orbital", meta = (ClampMin = "10.0", ClampMax = "5000.0", UIMin = "10.0", UIMax = "5000.0"))
	float OrbitalPullForce = 500.0f;

	//////////////////////////////////////////////////////////////////////////
	// Health Drain Parameters

	/** Rate at which health is drained from enemies per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Health", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float HealthDrainRate = 10.0f;

	/** Conversion ratio of enemy health to player health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Health", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HealthConversionRatio = 0.5f;

	/** Maximum distance for health drain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Health", meta = (ClampMin = "100.0", ClampMax = "2000.0", UIMin = "100.0", UIMax = "2000.0"))
	float HealthDrainDistance = 500.0f;

	//////////////////////////////////////////////////////////////////////////
	// Visual Effects Parameters

	/** Color of telekinesis energy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	FLinearColor TelekinesisColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f);

	/** Intensity of telekinesis glow effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects", meta = (ClampMin = "0.0", ClampMax = "20.0", UIMin = "0.0", UIMax = "20.0"))
	float GlowIntensity = 5.0f;

	/** Particle system for grabbed objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* GrabEffectTemplate;

	/** Particle system for thrown objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* ThrowEffectTemplate;

	/** Particle system for orbital objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* OrbitalEffectTemplate;

	/** Particle system for health drain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* HealthDrainEffectTemplate;

	/** Sound for grabbing objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* GrabSound;

	/** Sound for throwing objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* ThrowSound;

	/** Sound for health drain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* HealthDrainSound;

	/** Sound for orbital system activation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* OrbitalActivationSound;

private:
	/** Physics handle component for grabbing objects */
	UPhysicsHandleComponent* PhysicsHandle;

	/** Currently grabbed actor */
	AActor* GrabbedActor;

	/** Component being grabbed */
	UPrimitiveComponent* GrabbedComponent;

	/** Original linear damping of grabbed component */
	float OriginalLinearDamping;

	/** Original angular damping of grabbed component */
	float OriginalAngularDamping;

	/** Current throw charge time */
	float CurrentChargeTime;

	/** Whether currently charging a throw */
	bool bIsCharging;

	/** Whether the orbital system is active */
	bool bOrbitalSystemActive;

	/** Array of objects in the orbital system */
	TArray<AActor*> OrbitalObjects;

	/** Array of orbital angles for each object */
	TArray<float> OrbitalAngles;

	/** Array of orbital heights for each object */
	TArray<float> OrbitalHeights;

	/** Current orbital angle for positioning objects */
	float CurrentOrbitalAngle;

	/** Actor being health drained */
	AActor* HealthDrainTarget;

	/** Whether currently draining health */
	bool bIsDrainingHealth;

	/** Health component of the owner */
	UHealthComponent* OwnerHealthComponent;

	/** Timer handle for health drain */
	FTimerHandle HealthDrainTimerHandle;

	/** Performs a line trace to find an object to grab */
	bool FindObjectToGrab(FHitResult& OutHit);

	/** Performs a line trace to find an enemy to drain health from */
	bool FindHealthDrainTarget(FHitResult& OutHit);

	/** Finds nearby objects that can be added to the orbital system */
	void FindOrbitalObjects();

	/** Updates the positions of orbital objects */
	void UpdateOrbitalObjects(float DeltaTime);

	/** Drains health from the target */
	void DrainHealth();

	/** Handles impact events for thrown objects */
	UFUNCTION()
	void OnThrownObjectHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	/** Gets the owner's camera location and forward vector */
	void GetOwnerViewInfo(FVector& OutLocation, FVector& OutForwardVector);

	/** Performs a line trace to find a telekinesis object */
	bool TraceForTelekinesisObject(FHitResult& OutHit);

	/** Updates the grabbed object location */
	void UpdateGrabbedObjectLocation(float DeltaTime);

	/** Updates health drain effects */
	void UpdateHealthDrain(float DeltaTime);

	/** Gets the telekinesis direction */
	FVector GetTelekinesisDirection() const;

	/** Gets the telekinesis target location */
	FVector GetTelekinesisTargetLocation() const;
};