// Fill out your copyright notice in the Description page of Project Settings.

#include "TelekinesisComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UTelekinesisComponent::UTelekinesisComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize member variables
	GrabbedActor = nullptr;
	GrabbedComponent = nullptr;
	PhysicsHandle = nullptr;
	bIsCharging = false;
	CurrentChargeTime = 0.0f;
	bIsDrainingHealth = false;
	HealthDrainTarget = nullptr;
	bOrbitalSystemActive = false;
}

// Called when the game starts
void UTelekinesisComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get the physics handle component from the owner
	AActor* Owner = GetOwner();
	if (Owner)
	{
		PhysicsHandle = Owner->FindComponentByClass<UPhysicsHandleComponent>();
		if (!PhysicsHandle)
		{
			// Create a physics handle component if one doesn't exist
			PhysicsHandle = NewObject<UPhysicsHandleComponent>(Owner, TEXT("PhysicsHandle"));
			PhysicsHandle->RegisterComponent();
		}
	}
}

// Called every frame
void UTelekinesisComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update grabbed object position if we have one
	if (IsObjectGrabbed())
	{
		UpdateGrabbedObjectLocation(DeltaTime);
		UpdateOrbitalObjects(DeltaTime);
	}

	// Update throw charging
	if (bIsCharging)
	{
		CurrentChargeTime = FMath::Min(CurrentChargeTime + DeltaTime, MaxThrowChargeTime);
	}

	// Update health drain
	if (bIsDrainingHealth && HealthDrainTarget)
	{
		UpdateHealthDrain(DeltaTime);
	}
}

bool UTelekinesisComponent::GrabObject()
{
	// If already grabbing something, release it first
	if (IsObjectGrabbed())
	{
		ReleaseObject(false);
	}

	// Trace for an object to grab
	FHitResult HitResult;
	if (!TraceForTelekinesisObject(HitResult))
	{
		return false;
	}

	// Check if the hit component can be grabbed
	UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	if (!HitComponent || !CanGrabObject(HitComponent))
	{
		return false;
	}

	// Store references to the grabbed actor and component
	GrabbedActor = HitResult.GetActor();
	GrabbedComponent = HitComponent;

	// Grab the object with the physics handle
	if (PhysicsHandle && GrabbedComponent)
	{
		// Enable physics on the component if it's not already enabled
		if (!GrabbedComponent->IsSimulatingPhysics())
		{
			GrabbedComponent->SetSimulatePhysics(true);
		}

		// Grab the component at the hit location
		PhysicsHandle->GrabComponentAtLocationWithRotation(
			GrabbedComponent,
			NAME_None,
			HitResult.Location,
			GrabbedComponent->GetComponentRotation()
		);

		// Broadcast the object grabbed event
		OnObjectGrabbed.Broadcast(GrabbedActor);

		// Notify target changed
		OnTelekinesisTargetChanged(GrabbedActor, nullptr);

		return true;
	}

	// If we got here, something went wrong
	GrabbedActor = nullptr;
	GrabbedComponent = nullptr;
	return false;
}

void UTelekinesisComponent::ReleaseObject(bool bApplyThrowForce)
{
	if (!IsObjectGrabbed())
	{
		return;
	}

	// Store a reference to the object we're releasing
	AActor* ReleasedActor = GrabbedActor;
	UPrimitiveComponent* ReleasedComponent = GrabbedComponent;

	// Release the object from the physics handle
	if (PhysicsHandle)
	{
		PhysicsHandle->ReleaseComponent();

		// Apply throw force if requested
		if (bApplyThrowForce && ReleasedComponent && ReleasedComponent->IsSimulatingPhysics())
		{
			// Calculate throw force based on charge time
			float ChargeRatio = FMath::Clamp(CurrentChargeTime / MaxThrowChargeTime, 0.1f, 1.0f);
			FVector ThrowDirection = GetTelekinesisDirection();
			FVector ThrowForce = ThrowDirection * ThrowForceMultiplier * ChargeRatio;

			// Allow blueprint to modify the throw force
			ModifyThrowForce(ThrowForce, ChargeRatio, ReleasedActor);

			// Apply the impulse
			ReleasedComponent->AddImpulse(ThrowForce, NAME_None, true);

			// Broadcast the object thrown event
			OnObjectThrown.Broadcast(ReleasedActor, ThrowForce);
		}
	}

	// Clear orbital objects
	OrbitalObjects.Empty();
	OrbitalAngles.Empty();
	OrbitalHeights.Empty();

	// Broadcast the object released event
	OnObjectReleased.Broadcast(ReleasedActor);

	// Notify target changed
	OnTelekinesisTargetChanged(nullptr, ReleasedActor);

	// Reset charging state
	bIsCharging = false;
	CurrentChargeTime = 0.0f;

	// Clear references
	GrabbedActor = nullptr;
	GrabbedComponent = nullptr;
}

void UTelekinesisComponent::StartChargeThrow()
{
	if (IsObjectGrabbed())
	{
		bIsCharging = true;
		CurrentChargeTime = 0.0f;
	}
}

void UTelekinesisComponent::ThrowObject()
{
	if (IsObjectGrabbed())
	{
		ReleaseObject(true);
	}
}

void UTelekinesisComponent::MoveObjectToLocation(FVector TargetLocation)
{
	if (PhysicsHandle && IsObjectGrabbed())
	{
		PhysicsHandle->SetTargetLocation(TargetLocation);
	}
}

void UTelekinesisComponent::RotateObject(FRotator DeltaRotation)
{
	if (PhysicsHandle && IsObjectGrabbed() && GrabbedComponent)
	{
		// Get current rotation
		FRotator CurrentRotation = GrabbedComponent->GetComponentRotation();

		// Apply delta rotation
		FRotator NewRotation = CurrentRotation + DeltaRotation;

		// Set the target rotation
		PhysicsHandle->SetTargetRotation(NewRotation);
	}
}

void UTelekinesisComponent::StartHealthDrain()
{
	// Check if we're already draining health
	if (bIsDrainingHealth)
	{
		return;
	}

	// Trace for a potential health drain target
	FHitResult HitResult;
	if (!TraceForTelekinesisObject(HitResult))
	{
		return;
	}

	// Check if the hit actor can have health drained
	// For now, we'll just check if it's an AI character
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || !HitActor->IsA(ACharacter::StaticClass()) || HitActor == GetOwner())
	{
		return;
	}

	// Set the health drain target
	HealthDrainTarget = HitActor;
	bIsDrainingHealth = true;

	// Broadcast the health drain start event
	OnHealthDrainStart.Broadcast(HealthDrainTarget);

	// Notify target changed
	OnTelekinesisTargetChanged(HealthDrainTarget, nullptr);
}

void UTelekinesisComponent::StopHealthDrain()
{
	if (!bIsDrainingHealth || !HealthDrainTarget)
	{
		return;
	}

	// Store a reference to the target we were draining
	AActor* PreviousTarget = HealthDrainTarget;

	// Reset health drain state
	bIsDrainingHealth = false;
	HealthDrainTarget = nullptr;

	// Broadcast the health drain stop event
	OnHealthDrainStop.Broadcast(PreviousTarget);

	// Notify target changed
	OnTelekinesisTargetChanged(nullptr, PreviousTarget);
}

bool UTelekinesisComponent::CanGrabObject_Implementation(UPrimitiveComponent* Component)
{
	// Check if the component is valid and simulating physics
	if (!Component || !Component->IsSimulatingPhysics())
	{
		return false;
	}

	// Check if the component's mass is within our limit
	float Mass = Component->GetMass();
	if (Mass > MaxGrabbableMass)
	{
		return false;
	}

	return true;
}

void UTelekinesisComponent::ModifyThrowForce_Implementation(FVector& ThrowForce, float ChargeAmount, AActor* ThrownObject)
{
	// Default implementation does nothing, but can be overridden in blueprints
}

float UTelekinesisComponent::CalculateDamage_Implementation(AActor* HitActor, AActor* ThrownObject, float ImpactForce)
{
	// Default implementation calculates damage based on impact force
	return ImpactForce * 0.01f; // Scale down the force to a reasonable damage value
}

void UTelekinesisComponent::OnTelekinesisTargetChanged_Implementation(AActor* NewTarget, AActor* PreviousTarget)
{
	// Default implementation does nothing, but can be overridden in blueprints
}

bool UTelekinesisComponent::IsObjectGrabbed() const
{
	return (PhysicsHandle && PhysicsHandle->GrabbedComponent && GrabbedActor);
}

AActor* UTelekinesisComponent::GetGrabbedObject() const
{
	return GrabbedActor;
}

bool UTelekinesisComponent::IsCharging() const
{
	return bIsCharging;
}

float UTelekinesisComponent::GetCurrentChargeTime() const
{
	return CurrentChargeTime;
}

bool UTelekinesisComponent::IsDrainingHealth() const
{
	return bIsDrainingHealth;
}

AActor* UTelekinesisComponent::GetHealthDrainTarget() const
{
	return HealthDrainTarget;
}

FVector UTelekinesisComponent::GetTelekinesisDirection() const
{
	// Get the owner's controller
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ForwardVector;
	}

	AController* Controller = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		Controller = Character->GetController();
	}

	if (!Controller)
	{
		return Owner->GetActorForwardVector();
	}

	// Get the controller's view direction
	FRotator ControlRotation = Controller->GetControlRotation();
	return ControlRotation.Vector();
}

FVector UTelekinesisComponent::GetTelekinesisTargetLocation() const
{
	// Get the owner's location and forward vector
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	// Try to get the camera component if the owner is a character
	UCameraComponent* CameraComponent = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		// Find the camera component
		CameraComponent = Character->FindComponentByClass<UCameraComponent>();
	}

	// Use the camera location and rotation if available
	FVector StartLocation;
	FVector Direction;

	if (CameraComponent)
	{
		StartLocation = CameraComponent->GetComponentLocation();
		Direction = CameraComponent->GetForwardVector();
	}
	else
	{
		StartLocation = Owner->GetActorLocation();
		Direction = GetTelekinesisDirection();
	}

	// Calculate the target location at a distance in front of the player/camera
	return StartLocation + (Direction * 200.0f); // Default hold distance of 200 units
}

bool UTelekinesisComponent::TraceForTelekinesisObject(FHitResult& OutHit)
{
	// Get the owner's location and forward vector
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Try to get the camera component if the owner is a character
	UCameraComponent* CameraComponent = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		// Find the camera component
		CameraComponent = Character->FindComponentByClass<UCameraComponent>();
	}

	// Use the camera location and rotation if available
	FVector StartLocation;
	FVector Direction;

	if (CameraComponent)
	{
		StartLocation = CameraComponent->GetComponentLocation();
		Direction = CameraComponent->GetForwardVector();
	}
	else
	{
		StartLocation = Owner->GetActorLocation();
		Direction = GetTelekinesisDirection();
	}

	// Calculate the end location
	FVector EndLocation = StartLocation + (Direction * MaxGrabDistance);

	// Setup collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = false;

	// Perform the line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	// Draw debug line if in debug mode
	#if WITH_EDITOR
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 2.0f, 0, 1.0f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), OutHit.Location, 10.0f, 8, FColor::Red, false, 2.0f, 0, 1.0f);
		}
	#endif

	return bHit;
}

void UTelekinesisComponent::UpdateGrabbedObjectLocation(float DeltaTime)
{
	if (!PhysicsHandle || !IsObjectGrabbed())
	{
		return;
	}

	// Calculate the target location for the grabbed object
	FVector TargetLocation = GetTelekinesisTargetLocation();

	// Update the physics handle target location
	PhysicsHandle->SetTargetLocation(TargetLocation);
}

void UTelekinesisComponent::UpdateOrbitalObjects(float DeltaTime)
{
	if (!IsObjectGrabbed() || !GrabbedActor)
	{
		return;
	}

	// If we don't have any orbital objects yet, find some
	if (OrbitalObjects.Num() == 0)
	{
		// Find nearby physics objects that could orbit
		TArray<AActor*> NearbyActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), NearbyActors);

		for (AActor* Actor : NearbyActors)
		{
			// Skip the grabbed actor and the owner
			if (Actor == GrabbedActor || Actor == GetOwner())
			{
				continue;
			}

			// Check if the actor has a physics component
			UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
			if (!PrimComp || !PrimComp->IsSimulatingPhysics())
			{
				continue;
			}

			// Check if it's within orbital radius
			float Distance = FVector::Distance(Actor->GetActorLocation(), GrabbedActor->GetActorLocation());
			if (Distance > OrbitalRadius || Distance < 50.0f)
			{
				continue;
			}

			// Check if it's small enough to orbit (less than 1/3 the mass of the grabbed object)
			if (PrimComp->GetMass() > GrabbedComponent->GetMass() / 3.0f)
			{
				continue;
			}

			// Add to orbital objects
			OrbitalObjects.Add(Actor);
			
			// Assign random starting angle and height
			OrbitalAngles.Add(FMath::RandRange(0.0f, 360.0f));
			OrbitalHeights.Add(FMath::RandRange(-50.0f, 50.0f));

			// Limit to max orbital objects
			if (OrbitalObjects.Num() >= MaxOrbitalObjects)
			{
				break;
			}
		}

		// Broadcast orbital system activated if we found any objects
		if (OrbitalObjects.Num() > 0)
		{
			OnOrbitalSystemActivated.Broadcast(GrabbedActor);
		}
	}

	// Update orbital objects
	for (int32 i = 0; i < OrbitalObjects.Num(); ++i)
	{
		AActor* OrbitalActor = OrbitalObjects[i];
		if (!OrbitalActor)
		{
			continue;
		}

		UPrimitiveComponent* OrbitalComp = Cast<UPrimitiveComponent>(OrbitalActor->GetRootComponent());
		if (!OrbitalComp || !OrbitalComp->IsSimulatingPhysics())
		{
			continue;
		}

		// Update the orbital angle
		OrbitalAngles[i] += OrbitalSpeed * DeltaTime;
		if (OrbitalAngles[i] > 360.0f)
		{
			OrbitalAngles[i] -= 360.0f;
		}

		// Calculate the orbital position
		float Angle = FMath::DegreesToRadians(OrbitalAngles[i]);
		FVector CenterLocation = GrabbedActor->GetActorLocation();
		FVector OrbitalOffset = FVector(
			FMath::Cos(Angle) * OrbitalRadius,
			FMath::Sin(Angle) * OrbitalRadius,
			OrbitalHeights[i]
		);

		// Rotate the offset to align with the grabbed actor's rotation
		FRotator GrabbedRotation = GrabbedActor->GetActorRotation();
		OrbitalOffset = GrabbedRotation.RotateVector(OrbitalOffset);

		// Calculate the target position
		FVector TargetPosition = CenterLocation + OrbitalOffset;

		// Apply force to move the orbital object toward the target position
		FVector CurrentLocation = OrbitalActor->GetActorLocation();
		FVector DirectionToTarget = (TargetPosition - CurrentLocation);
		float DistanceToTarget = DirectionToTarget.Size();

		// Only apply force if we're not too close to the target
		if (DistanceToTarget > 10.0f)
		{
			DirectionToTarget.Normalize();
			
			// Calculate force based on distance (stronger when further away)
			float ForceStrength = FMath::Clamp(DistanceToTarget * 2.0f, 100.0f, 2000.0f);
			
			// Apply the force
			OrbitalComp->AddForce(DirectionToTarget * ForceStrength, NAME_None, true);

			// Add a small upward force to counteract gravity
			OrbitalComp->AddForce(FVector(0, 0, OrbitalComp->GetMass() * 980.0f), NAME_None, true);
		}
	}
}

void UTelekinesisComponent::UpdateHealthDrain(float DeltaTime)
{
	if (!bIsDrainingHealth || !HealthDrainTarget)
	{
		return;
	}

	// Check if the target is still in range
	float Distance = FVector::Distance(GetOwner()->GetActorLocation(), HealthDrainTarget->GetActorLocation());
	if (Distance > MaxGrabDistance)
	{
		StopHealthDrain();
		return;
	}

	// Calculate the amount of health to drain this frame
	float DrainAmount = HealthDrainRate * DeltaTime;

	// Apply damage to the target
	// Note: This is a placeholder. In a real implementation, you would use the damage system
	// and interface with a health component on the target.
	FDamageEvent DamageEvent;
	HealthDrainTarget->TakeDamage(DrainAmount, DamageEvent, GetOwner()->GetInstigatorController(), GetOwner());

	// Heal the player
	// Note: This is a placeholder. In a real implementation, you would interface with a health component
	// on the player character.
	float HealAmount = DrainAmount * HealthConversionRatio;
	// Apply healing to the owner (implementation depends on your health system)
}

// Implementation of EndPlay
void UTelekinesisComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Release any grabbed object
	if (IsObjectGrabbed())
	{
		ReleaseObject(false);
	}

	// Stop health drain
	if (bIsDrainingHealth)
	{
		StopHealthDrain();
	}

	// Deactivate orbital system
	if (bOrbitalSystemActive)
	{
		DeactivateOrbitalSystem();
	}
}

// Implementation of GetChargePercentage
float UTelekinesisComponent::GetChargePercentage() const
{
	if (!bIsCharging || MaxThrowChargeTime <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(CurrentChargeTime / MaxThrowChargeTime, 0.0f, 1.0f);
}

// Implementation of ActivateOrbitalSystem
void UTelekinesisComponent::ActivateOrbitalSystem()
{
	if (!IsObjectGrabbed() || bOrbitalSystemActive)
	{
		return;
	}

	bOrbitalSystemActive = true;

	// Find orbital objects
	TArray<AActor*> NearbyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), NearbyActors);

	for (AActor* Actor : NearbyActors)
	{
		// Skip the grabbed actor and the owner
		if (Actor == GrabbedActor || Actor == GetOwner())
		{
			continue;
		}

		// Check if the actor has a physics component
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
		if (!PrimComp || !PrimComp->IsSimulatingPhysics())
		{
			continue;
		}

		// Check if it's within orbital radius
		float Distance = FVector::Distance(Actor->GetActorLocation(), GrabbedActor->GetActorLocation());
		if (Distance > OrbitalRadius || Distance < 50.0f)
		{
			continue;
		}

		// Check if it's small enough to orbit
		if (PrimComp->GetMass() > MaxOrbitalObjectMass)
		{
			continue;
		}

		// Add to orbital objects
		OrbitalObjects.Add(Actor);
		
		// Assign random starting angle and height
		OrbitalAngles.Add(FMath::RandRange(0.0f, 360.0f));
		OrbitalHeights.Add(FMath::RandRange(-50.0f, 50.0f));

		// Limit to max orbital objects
		if (OrbitalObjects.Num() >= MaxOrbitalObjects)
		{
			break;
		}
	}

	// Broadcast orbital system activated if we found any objects
	if (OrbitalObjects.Num() > 0)
	{
		OnOrbitalSystemActivated.Broadcast(GrabbedActor);
	}
}

// Implementation of DeactivateOrbitalSystem
void UTelekinesisComponent::DeactivateOrbitalSystem()
{
	if (!bOrbitalSystemActive)
	{
		return;
	}

	bOrbitalSystemActive = false;

	// Clear orbital objects
	OrbitalObjects.Empty();
	OrbitalAngles.Empty();
	OrbitalHeights.Empty();
}

// Implementation of IsOrbitalSystemActive
bool UTelekinesisComponent::IsOrbitalSystemActive() const
{
	return bOrbitalSystemActive;
}

// Implementation of GetOrbitalObjects
TArray<AActor*> UTelekinesisComponent::GetOrbitalObjects() const
{
	return OrbitalObjects;
}

// Implementation of OnThrownObjectHit
void UTelekinesisComponent::OnThrownObjectHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!SelfActor || !OtherActor || SelfActor == OtherActor)
	{
		return;
	}

	// Calculate impact force
	float ImpactForce = NormalImpulse.Size();

	// Calculate damage based on impact force
	float Damage = CalculateDamage(OtherActor, SelfActor, ImpactForce);

	// Apply damage if significant
	if (Damage > 0.0f)
	{
		FDamageEvent DamageEvent;
		OtherActor->TakeDamage(Damage, DamageEvent, nullptr, SelfActor);
	}

	// Broadcast impact event
	OnObjectImpact.Broadcast(SelfActor, Hit.ImpactPoint);
}