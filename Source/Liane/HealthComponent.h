// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, HealthDelta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LIANE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Health properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0"))
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bInvulnerable = false;

	// Health regeneration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration")
	bool bRegenHealth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (EditCondition = "bRegenHealth"))
	float HealthRegenRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (EditCondition = "bRegenHealth"))
	float HealthRegenDelay = 3.0f;

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthDepleted OnHealthDepleted;

	// Health functions
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float TakeDamage(float DamageAmount, bool bIgnoreInvulnerability = false);

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float HealDamage(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual void SetCurrentHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual bool IsDead() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual void ResetHealth();

	// Blueprint implementable events
	UFUNCTION(BlueprintNativeEvent, Category = "Health")
	void OnDamaged(float DamageAmount);
	void OnDamaged_Implementation(float DamageAmount);

	UFUNCTION(BlueprintNativeEvent, Category = "Health")
	void OnHealed(float HealAmount);
	void OnHealed_Implementation(float HealAmount);

	UFUNCTION(BlueprintNativeEvent, Category = "Health")
	void OnDeath();
	void OnDeath_Implementation();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Health regeneration timer
	float TimeSinceLastDamage = 0.0f;
	bool bIsRegenerating = false;

	// Handle damage from the engine's damage system
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
};