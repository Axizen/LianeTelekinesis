// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize health to max health
	CurrentHealth = MaxHealth;

	// Register for damage events
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle health regeneration
	if (bRegenHealth && !IsDead())
	{
		if (CurrentHealth < MaxHealth)
		{
			// Increment time since last damage
			TimeSinceLastDamage += DeltaTime;

			// Check if we should start regenerating
			if (TimeSinceLastDamage >= HealthRegenDelay)
			{
				// Calculate heal amount for this frame
				float HealAmount = HealthRegenRate * DeltaTime;
				
				// Apply healing
				HealDamage(HealAmount);
			}
		}
	}
}

float UHealthComponent::GetHealthPercent() const
{
	return (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;
}

float UHealthComponent::TakeDamage(float DamageAmount, bool bIgnoreInvulnerability)
{
	// Check if we can take damage
	if (IsDead() || (bInvulnerable && !bIgnoreInvulnerability) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// Calculate new health
	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	// Calculate actual damage dealt
	float ActualDamage = OldHealth - CurrentHealth;

	// Reset regeneration timer
	TimeSinceLastDamage = 0.0f;
	bIsRegenerating = false;

	// Call damage event
	OnDamaged(ActualDamage);

	// Broadcast health changed event
	OnHealthChanged.Broadcast(CurrentHealth, -ActualDamage);

	// Check for death
	if (CurrentHealth <= 0.0f)
	{
		OnDeath();
		OnHealthDepleted.Broadcast();
	}

	return ActualDamage;
}

float UHealthComponent::HealDamage(float HealAmount)
{
	// Check if we can heal
	if (IsDead() || HealAmount <= 0.0f || CurrentHealth >= MaxHealth)
	{
		return 0.0f;
	}

	// Calculate new health
	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);

	// Calculate actual healing done
	float ActualHeal = CurrentHealth - OldHealth;

	// Call heal event
	OnHealed(ActualHeal);

	// Broadcast health changed event
	OnHealthChanged.Broadcast(CurrentHealth, ActualHeal);

	return ActualHeal;
}

void UHealthComponent::SetCurrentHealth(float NewHealth)
{
	// Calculate health delta
	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	float HealthDelta = CurrentHealth - OldHealth;

	// Broadcast health changed event
	OnHealthChanged.Broadcast(CurrentHealth, HealthDelta);

	// Check for death
	if (CurrentHealth <= 0.0f && OldHealth > 0.0f)
	{
		OnDeath();
		OnHealthDepleted.Broadcast();
	}
	// Check for resurrection
	else if (CurrentHealth > 0.0f && OldHealth <= 0.0f)
	{
		// Handle resurrection logic if needed
	}
}

bool UHealthComponent::IsDead() const
{
	return CurrentHealth <= 0.0f;
}

void UHealthComponent::ResetHealth()
{
	// Reset health to max
	SetCurrentHealth(MaxHealth);

	// Reset regeneration timer
	TimeSinceLastDamage = 0.0f;
	bIsRegenerating = false;
}

void UHealthComponent::OnDamaged_Implementation(float DamageAmount)
{
	// Default implementation does nothing
}

void UHealthComponent::OnHealed_Implementation(float HealAmount)
{
	// Default implementation does nothing
}

void UHealthComponent::OnDeath_Implementation()
{
	// Default implementation - check if owner is a character and disable movement
	AActor* Owner = GetOwner();
	if (Owner)
	{
		ACharacter* Character = Cast<ACharacter>(Owner);
		if (Character)
		{
			// Disable character movement
			UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
			if (MovementComponent)
			{
				MovementComponent->DisableMovement();
			}

			// Disable collision
			Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// Forward damage to our TakeDamage function
	TakeDamage(Damage);
}