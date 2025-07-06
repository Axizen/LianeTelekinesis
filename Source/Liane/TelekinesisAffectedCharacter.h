// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TelekinesisInteractable.h"
#include "TelekinesisAffectedCharacter.generated.h"

class UHealthComponent;

UCLASS()
class LIANE_API ATelekinesisAffectedCharacter : public ACharacter, public ITelekinesisInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATelekinesisAffectedCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Health component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	// Telekinesis properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	float TelekinesisMassOverride = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	bool bCanBeGrabbed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	float HealthDrainResistance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Ragdoll")
	float MinImpactForceForRagdoll = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Ragdoll")
	float RagdollRecoveryTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Ragdoll")
	bool bCanRecoverFromRagdoll = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* GrabEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* ThrowEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* ImpactEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* HealthDrainEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* GrabSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* ThrowSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* HealthDrainSound;

	// ITelekinesisInteractable interface implementation
	virtual bool CanBeGrabbed_Implementation() const override;
	virtual void OnGrabbed_Implementation(AActor* GrabbedBy) override;
	virtual void OnReleased_Implementation(AActor* ReleasedBy, bool bThrowPerformed) override;
	virtual void OnThrown_Implementation(AActor* ThrownBy, FVector ThrowForce) override;
	virtual float GetTelekinesisMass_Implementation() const override;
	virtual void ApplyTelekineticForce_Implementation(FVector Force, FName BoneName) override;
	virtual bool CanOrbit_Implementation() const override;
	virtual void OnOrbitStart_Implementation(AActor* OrbitAround) override;
	virtual void OnOrbitEnd_Implementation() override;
	virtual void OnTelekineticImpact_Implementation(AActor* HitActor, FVector ImpactPoint, FVector ImpactNormal, float ImpactForce) override;

	// Health drain functions
	UFUNCTION(BlueprintCallable, Category = "Telekinesis")
	virtual void StartHealthDrain(AActor* DrainedBy);

	UFUNCTION(BlueprintCallable, Category = "Telekinesis")
	virtual void StopHealthDrain();

	UFUNCTION(BlueprintCallable, Category = "Telekinesis")
	virtual float DrainHealth(float DrainAmount, AActor* DrainedBy);

	// Ragdoll functions
	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Ragdoll")
	virtual void EnterRagdollState();

	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Ragdoll")
	virtual void RecoverFromRagdoll();

	UFUNCTION(BlueprintCallable, Category = "Telekinesis|Ragdoll")
	bool IsRagdolled() const { return bIsRagdolled; }

	// Blueprint implementable events
	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnGrabbed(AActor* GrabbedBy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnReleased(AActor* ReleasedBy, bool bThrowPerformed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnThrown(AActor* ThrownBy, FVector ThrowForce);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnImpact(AActor* HitActor, FVector ImpactPoint, FVector ImpactNormal, float ImpactForce);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnHealthDrainStart(AActor* DrainedBy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnHealthDrainStop();

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnEnterRagdoll();

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnRecoverFromRagdoll();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Handle collision events
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Telekinesis state
	bool bIsGrabbed;
	bool bWasThrown;
	AActor* GrabbedByActor;

	// Health drain state
	bool bIsBeingDrained;
	AActor* DrainedByActor;
	UParticleSystemComponent* HealthDrainEffectComponent;

	// Ragdoll state
	bool bIsRagdolled;
	float RagdollRecoveryTimer;

	// Particle components for effects
	UPROPERTY()
	UParticleSystemComponent* GrabEffectComponent;

	// Timer handle for ragdoll recovery
	FTimerHandle RagdollRecoveryTimerHandle;
};