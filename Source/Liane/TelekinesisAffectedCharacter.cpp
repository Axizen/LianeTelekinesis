// Fill out your copyright notice in the Description page of Project Settings.

#include "TelekinesisAffectedCharacter.h"
#include "HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "AIController.h"
#include "BrainComponent.h"

// Sets default values
ATelekinesisAffectedCharacter::ATelekinesisAffectedCharacter()
{
	// Set this character to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Create health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Setup collision
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true); // Enable hit notifications
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// Initialize telekinesis state
	bIsGrabbed = false;
	bWasThrown = false;
	GrabbedByActor = nullptr;

	// Initialize health drain state
	bIsBeingDrained = false;
	DrainedByActor = nullptr;
	HealthDrainEffectComponent = nullptr;

	// Initialize ragdoll state
	bIsRagdolled = false;
	RagdollRecoveryTimer = 0.0f;
	GrabEffectComponent = nullptr;
}

// Called when the game starts or when spawned
void ATelekinesisAffectedCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind hit event
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ATelekinesisAffectedCharacter::OnHit);
}

// Called every frame
void ATelekinesisAffectedCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update grab effect position if active
	if (GrabEffectComponent && GrabEffectComponent->IsActive())
	{
		GrabEffectComponent->SetWorldLocation(GetActorLocation());
	}

	// Update health drain effect position if active
	if (HealthDrainEffectComponent && HealthDrainEffectComponent->IsActive())
	{
		HealthDrainEffectComponent->SetWorldLocation(GetActorLocation());
	}

	// Handle ragdoll recovery
	if (bIsRagdolled && bCanRecoverFromRagdoll && !HealthComponent->IsDead())
	{
		// Only start recovery timer if we're not being grabbed
		if (!bIsGrabbed)
		{
			RagdollRecoveryTimer += DeltaTime;

			// Check if it's time to recover
			if (RagdollRecoveryTimer >= RagdollRecoveryTime)
			{
				RecoverFromRagdoll();
			}
		}
	}
}

// Called to bind functionality to input
void ATelekinesisAffectedCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// ITelekinesisInteractable interface implementation
bool ATelekinesisAffectedCharacter::CanBeGrabbed_Implementation() const
{
	return bCanBeGrabbed && !HealthComponent->IsDead();
}

void ATelekinesisAffectedCharacter::OnGrabbed_Implementation(AActor* GrabbedBy)
{
	// Set grabbed state
	bIsGrabbed = true;
	bWasThrown = false;
	GrabbedByActor = GrabbedBy;

	// Enter ragdoll state
	EnterRagdollState();

	// Spawn grab effect if specified
	if (GrabEffectTemplate)
	{
		GrabEffectComponent = UGameplayStatics::SpawnEmitterAttached(
			GrabEffectTemplate,
			GetMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			FVector(1.0f),
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// Play grab sound if specified
	if (GrabSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			GrabSound,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	// Disable AI while grabbed
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->BrainComponent)
	{
		AIController->BrainComponent->StopLogic(TEXT("Grabbed by telekinesis"));
	}

	// Call blueprint event
	BP_OnGrabbed(GrabbedBy);
}

void ATelekinesisAffectedCharacter::OnReleased_Implementation(AActor* ReleasedBy, bool bThrowPerformed)
{
	// Set released state
	bIsGrabbed = false;
	this->bWasThrown = bThrowPerformed;

	// Stop grab effect if active
	if (GrabEffectComponent && GrabEffectComponent->IsActive())
	{
		GrabEffectComponent->DeactivateSystem();
		GrabEffectComponent = nullptr;
	}

	// Reset ragdoll recovery timer if not thrown
	if (!bWasThrown)
	{
		RagdollRecoveryTimer = RagdollRecoveryTime; // Force immediate recovery
	}

	// Call blueprint event
	BP_OnReleased(ReleasedBy, bWasThrown);

	// Clear references
	GrabbedByActor = nullptr;
}

void ATelekinesisAffectedCharacter::OnThrown_Implementation(AActor* ThrownBy, FVector ThrowForce)
{
	// Set thrown state
	bWasThrown = true;

	// Apply the throw force
	GetMesh()->AddImpulse(ThrowForce, NAME_None, true);

	// Spawn throw effect if specified
	if (ThrowEffectTemplate)
	{
		UGameplayStatics::SpawnEmitterAttached(
			ThrowEffectTemplate,
			GetMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			FVector(1.0f),
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// Play throw sound if specified
	if (ThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ThrowSound,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	// Call blueprint event
	BP_OnThrown(ThrownBy, ThrowForce);
}

float ATelekinesisAffectedCharacter::GetTelekinesisMass_Implementation() const
{
	// Use override mass if specified
	if (TelekinesisMassOverride > 0.0f)
	{
		return TelekinesisMassOverride;
	}

	// Otherwise use a mass based on the character's size
	return GetCapsuleComponent()->GetScaledCapsuleRadius() * GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.01f;
}

void ATelekinesisAffectedCharacter::ApplyTelekineticForce_Implementation(FVector Force, FName BoneName)
{
	// Apply force to the mesh
	if (GetMesh()->IsSimulatingPhysics(BoneName))
	{
		GetMesh()->AddForce(Force, BoneName, true);
	}
	else
	{
		// If the specified bone is not simulating physics, apply to the root bone
		GetMesh()->AddForce(Force, NAME_None, true);
	}
}

bool ATelekinesisAffectedCharacter::CanOrbit_Implementation() const
{
	// Characters are too large to orbit
	return false;
}

void ATelekinesisAffectedCharacter::OnOrbitStart_Implementation(AActor* OrbitAround)
{
	// Characters don't orbit
}

void ATelekinesisAffectedCharacter::OnOrbitEnd_Implementation()
{
	// Characters don't orbit
}

void ATelekinesisAffectedCharacter::OnTelekineticImpact_Implementation(AActor* HitActor, FVector ImpactPoint, FVector ImpactNormal, float ImpactForce)
{
	// Spawn impact effects
	if (ImpactEffectTemplate)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactEffectTemplate,
			ImpactPoint,
			FRotationMatrix::MakeFromZ(ImpactNormal).Rotator(),
			true
		);
	}

	// Play impact sound if specified
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ImpactSound,
			ImpactPoint,
			FRotator::ZeroRotator
		);
	}

	// Apply damage to self based on impact force
	if (ImpactForce >= MinImpactForceForRagdoll)
	{
		// Calculate damage based on impact force
		float Damage = ImpactForce * 0.0005f;

		// Apply damage to self
		if (HealthComponent)
		{
			HealthComponent->TakeDamage(Damage);
		}
	}

	// Call blueprint event
	BP_OnImpact(HitActor, ImpactPoint, ImpactNormal, ImpactForce);
}

void ATelekinesisAffectedCharacter::StartHealthDrain(AActor* DrainedBy)
{
	// Check if we can be drained
	if (bIsBeingDrained || HealthComponent->IsDead())
	{
		return;
	}

	// Set health drain state
	bIsBeingDrained = true;
	DrainedByActor = DrainedBy;

	// Spawn health drain effect if specified
	if (HealthDrainEffectTemplate)
	{
		HealthDrainEffectComponent = UGameplayStatics::SpawnEmitterAttached(
			HealthDrainEffectTemplate,
			GetMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			FVector(1.0f),
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// Play health drain sound if specified
	if (HealthDrainSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HealthDrainSound,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	// Call blueprint event
	BP_OnHealthDrainStart(DrainedBy);
}

void ATelekinesisAffectedCharacter::StopHealthDrain()
{
	// Check if we're being drained
	if (!bIsBeingDrained)
	{
		return;
	}

	// Reset health drain state
	bIsBeingDrained = false;
	DrainedByActor = nullptr;

	// Stop health drain effect if active
	if (HealthDrainEffectComponent && HealthDrainEffectComponent->IsActive())
	{
		HealthDrainEffectComponent->DeactivateSystem();
		HealthDrainEffectComponent = nullptr;
	}

	// Call blueprint event
	BP_OnHealthDrainStop();
}

float ATelekinesisAffectedCharacter::DrainHealth(float DrainAmount, AActor* DrainedBy)
{
	// Check if we can be drained
	if (!bIsBeingDrained || HealthComponent->IsDead())
	{
		return 0.0f;
	}

	// Apply resistance to drain amount
	float AdjustedDrainAmount = DrainAmount / HealthDrainResistance;

	// Apply damage to self
	if (HealthComponent)
	{
		return HealthComponent->TakeDamage(AdjustedDrainAmount);
	}

	return 0.0f;
}

void ATelekinesisAffectedCharacter::EnterRagdollState()
{
	// Check if we're already ragdolled
	if (bIsRagdolled)
	{
		return;
	}

	// Set ragdoll state
	bIsRagdolled = true;
	RagdollRecoveryTimer = 0.0f;

	// Disable capsule collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Enable mesh physics
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();

	// Disable character movement
	GetCharacterMovement()->DisableMovement();

	// Disable AI while ragdolled
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->BrainComponent)
	{
		AIController->BrainComponent->StopLogic(TEXT("Ragdolled"));
	}

	// Call blueprint event
	BP_OnEnterRagdoll();
}

void ATelekinesisAffectedCharacter::RecoverFromRagdoll()
{
	// Check if we're ragdolled and can recover
	if (!bIsRagdolled || HealthComponent->IsDead())
	{
		return;
	}

	// Reset ragdoll state
	bIsRagdolled = false;
	RagdollRecoveryTimer = 0.0f;

	// Re-enable capsule collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Disable mesh physics
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetSimulatePhysics(false);

	// Re-enable character movement
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// Align capsule with mesh
	FVector NewLocation = GetMesh()->GetComponentLocation();
	NewLocation.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(NewLocation);

	// Reset mesh relative location
	GetMesh()->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));

	// Restart AI
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->BrainComponent)
	{
		AIController->BrainComponent->RestartLogic();
	}

	// Call blueprint event
	BP_OnRecoverFromRagdoll();
}

void ATelekinesisAffectedCharacter::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only process impacts if we were thrown or ragdolled
	if ((!bWasThrown && !bIsRagdolled) || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Calculate impact force
	float ImpactForce = NormalImpulse.Size();

	// If the impact force is significant, process it
	if (ImpactForce >= MinImpactForceForRagdoll)
	{
		OnTelekineticImpact(OtherActor, Hit.ImpactPoint, Hit.ImpactNormal, ImpactForce);
		
		// Reset thrown state after significant impact
		bWasThrown = false;
	}
}