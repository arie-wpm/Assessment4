// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableObject.h"   

#include "AGP/GoalActionOrientatedPlanning/GOAP base/Agent.h"
// Sets default values
AInteractableObject::AInteractableObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AInteractableObject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AInteractableObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AInteractableObject::Interact(AActor* InteractingAgent)
{
	if (InteractingAgent)
	{
		UAgent* Agent = InteractingAgent->FindComponentByClass<UAgent>();

		if(Agent)
		{
			UE_LOG(LogTemp, Log, TEXT("Object Action executed"));
		}
	}
}

bool AInteractableObject::CanInteract(AActor* InteractingAgent)
{
	return true;
}

