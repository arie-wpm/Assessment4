// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GOAP base/Action.h"
#include "HealAction.generated.h"

/**
 * 
 */
UCLASS()
class AGP_API UHealAction : public UAction
{
	GENERATED_BODY()
	UHealAction();
	virtual bool IsActionPossible(const UWorldState& WorldState, const UBeliefs& Beliefs)override;

	// Chase the thief (move towards the last known location)
	virtual void PerformAction() override;

	// Check if the action is complete (e.g., lost sight of thief)
	virtual bool IsActionComplete() const override;

	// Apply effects to the world state (e.g., thief is no longer visible after chase)
	virtual void ApplyEffects(UWorldState& WorldState) override;
	
	
};
