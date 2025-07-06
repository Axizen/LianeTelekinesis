// Fill out your copyright notice in the Description page of Project Settings.

#include "TelekineticObject.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"

// Sets default values
ATelekineticObject::ATelekineticObject()
{
	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Create the static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// Setup physics
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetNotifyRigidBodyCollision(true); // Enable hit notifications
	MeshComponent->SetGenerateOverlapEvents(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));

	// Initialize telekinesis state
	bIsGrabbed = false;
	bWasThrown = false;
	bIsOrbiting = false;
	GrabbedByActor = nullptr;
	OrbitAroundActor = nullptr;
	GrabEffectComponent = nullptr;
}

// Called when the game starts or when spawned
void ATelekineticObject::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind hit event
	MeshComponent->OnComponentHit.AddDynamic(this, &ATelekineticObject::OnHit);
}

// Called every frame
void ATelekineticObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update grab effect position if active
	if (GrabEffectComponent && GrabEffectComponent->IsActive())
	{
		GrabEffectComponent->SetWorldLocation(MeshComponent->GetComponentLocation());
	}
}

// ITelekinesisInteractable interface implementation
bool ATelekineticObject::CanBeGrabbed_Implementation() const
{
	return bCanBeGrabbed && MeshComponent && MeshComponent->IsSimulatingPhysics();
}

void ATelekineticObject::OnGrabbed_Implementation(AActor* GrabbedBy)
{
	// Set grabbed state
	bIsGrabbed = true;
	bWasThrown = false;
	GrabbedByActor = GrabbedBy;

	// Spawn grab effect if specified
	if (GrabEffectTemplate)
	{
		GrabEffectComponent = UGameplayStatics::SpawnEmitterAttached(
			GrabEffectTemplate,
			MeshComponent,
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

	// Call blueprint event
	BP_OnGrabbed(GrabbedBy);
}

void ATelekineticObject::OnReleased_Implementation(AActor* ReleasedBy, bool bThrowPerformed)
{
	// Set released state
	bIsGrabbed = false;
	bWasThrown = bThrowPerformed;

	// Stop grab effect if active
	if (GrabEffectComponent && GrabEffectComponent->IsActive())
	{
		GrabEffectComponent->DeactivateSystem();
		GrabEffectComponent = nullptr;
	}

	// Call blueprint event
	BP_OnReleased(ReleasedBy, bWasThrown);

	// Clear references
	GrabbedByActor = nullptr;
}

void ATelekineticObject::OnThrown_Implementation(AActor* ThrownBy, FVector ThrowForce)
{
	// Set thrown state
	bWasThrown = true;

	// Spawn throw effect if specified
	if (ThrowEffectTemplate)
	{
		UGameplayStatics::SpawnEmitterAttached(
			ThrowEffectTemplate,
			MeshComponent,
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

float ATelekineticObject::GetTelekinesisMass_Implementation() const
{
	// Use override mass if specified
	if (TelekinesisMassOverride > 0.0f)
	{
		return TelekinesisMassOverride;
	}

	// Otherwise use the physics mass
	if (MeshComponent && MeshComponent->IsSimulatingPhysics())
	{
		return MeshComponent->GetMass();
	}

	return 10.0f; // Default mass
}

void ATelekineticObject::ApplyTelekineticForce_Implementation(FVector Force, FName BoneName)
{
	// Apply force to the mesh component
	if (MeshComponent && MeshComponent->IsSimulatingPhysics())
	{
		MeshComponent->AddForce(Force, BoneName, true);
	}
}

bool ATelekineticObject::CanOrbit_Implementation() const
{
	return bCanOrbit && !bIsGrabbed && MeshComponent && MeshComponent->IsSimulatingPhysics();
}

void ATelekineticObject::OnOrbitStart_Implementation(AActor* OrbitAround)
{
	// Set orbiting state
	bIsOrbiting = true;
	OrbitAroundActor = OrbitAround;

	// Call blueprint event
	BP_OnOrbitStart(OrbitAround);
}

void ATelekineticObject::OnOrbitEnd_Implementation()
{
	// Clear orbiting state
	bIsOrbiting = false;
	OrbitAroundActor = nullptr;

	// Call blueprint event
	BP_OnOrbitEnd();
}

void ATelekineticObject::OnTelekineticImpact_Implementation(AActor* HitActor, FVector ImpactPoint, FVector ImpactNormal, float ImpactForce)
{
	// Apply damage to the hit actor if the impact force is high enough
	if (ImpactForce >= MinImpactForceForDamage)
	{
		ApplyDamageOnImpact(HitActor, ImpactForce);
	}

	// Spawn impact effects
	SpawnImpactEffects(ImpactPoint, ImpactNormal);

	// Call blueprint event
	BP_OnImpact(HitActor, ImpactPoint, ImpactNormal, ImpactForce);
}

void ATelekineticObject::ApplyDamageOnImpact(AActor* HitActor, float ImpactForce)
{
	if (!HitActor || ImpactForce < MinImpactForceForDamage)
	{
		return;
	}

	// Calculate damage based on impact force and mass
	float Mass = GetTelekinesisMass();
	float Damage = (ImpactForce * Mass * 0.0001f) * DamageMultiplier;

	// Apply damage to the hit actor
	AController* InstigatorController = nullptr;
	if (GrabbedByActor)
	{
		ACharacter* Character = Cast<ACharacter>(GrabbedByActor);
		if (Character)
		{
			InstigatorController = Character->GetController();
		}
	}

	UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController, this, UDamageType::StaticClass());
}

void ATelekineticObject::SpawnImpactEffects(FVector ImpactPoint, FVector ImpactNormal)
{
	// Spawn impact effect if specified
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
}

void ATelekineticObject::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only process impacts if we were thrown
	if (!bWasThrown || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Calculate impact force
	float ImpactForce = NormalImpulse.Size();

	// If the impact force is significant, process it
	if (ImpactForce >= MinImpactForceForDamage)
	{
		OnTelekineticImpact(OtherActor, Hit.ImpactPoint, Hit.ImpactNormal, ImpactForce);
		
		// Reset thrown state after significant impact
		bWasThrown = false;
	}
}