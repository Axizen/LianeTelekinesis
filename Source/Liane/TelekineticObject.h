// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TelekinesisInteractable.h"
#include "TelekineticObject.generated.h"

UCLASS()
class LIANE_API ATelekineticObject : public AActor, public ITelekinesisInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATelekineticObject();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Static mesh component for the object
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// Telekinesis properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	float TelekinesisMassOverride = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	bool bCanBeGrabbed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	bool bCanOrbit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis")
	float MinImpactForceForDamage = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* GrabEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* ThrowEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	UParticleSystem* ImpactEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* GrabSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* ThrowSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Effects")
	USoundBase* ImpactSound;

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

	// Blueprint implementable events for telekinesis effects
	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnGrabbed(AActor* GrabbedBy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnReleased(AActor* ReleasedBy, bool bThrowPerformed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnThrown(AActor* ThrownBy, FVector ThrowForce);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnImpact(AActor* HitActor, FVector ImpactPoint, FVector ImpactNormal, float ImpactForce);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnOrbitStart(AActor* OrbitAround);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telekinesis")
	void BP_OnOrbitEnd();

	// Helper functions
	UFUNCTION(BlueprintCallable, Category = "Telekinesis")
	void ApplyDamageOnImpact(AActor* HitActor, float ImpactForce);

	UFUNCTION(BlueprintCallable, Category = "Telekinesis")
	void SpawnImpactEffects(FVector ImpactPoint, FVector ImpactNormal);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Handle collision events
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Telekinesis state
	bool bIsGrabbed;
	bool bWasThrown;
	bool bIsOrbiting;
	AActor* GrabbedByActor;
	AActor* OrbitAroundActor;

	// Particle components for effects
	UPROPERTY()
	UParticleSystemComponent* GrabEffectComponent;
};