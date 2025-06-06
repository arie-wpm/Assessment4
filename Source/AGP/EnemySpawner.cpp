// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"

#include "MultiplayerGameMode.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/BaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Pathfinding/PathfindingSubsystem.h"

// Sets default values
AEnemySpawner::AEnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->SetIsReplicated(true);
	bAlwaysRelevant = true;
	bReplicates = true;

}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	//Set the spawn parameters to ignore collision; this is so that spawns always happen. Also Get a reference to the player. 
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(),0));
	PopulateSpawnLocations();
	UE_LOG(LogTemp, Log, TEXT("Started"));
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Using Player kill count, determine how many times to spawn an enemy. Use a timer to determine when spawns will happen. 
	if (SpawnTimer <= 0.0f)
	{
		for (int8 i = 0; i < GetEnemySpawnAmount(PlayerCharacter->GetEnemiesKilledInLastMinute()); i++)
		{
			if (HasAuthority())
			{
				SpawnEnemy();
			}	
		}
		SpawnTimer += 5.0f;
	}

	if (PossibleSpawnLocations.IsEmpty())
	{
		PopulateSpawnLocations();
	}

	SpawnTimer -= DeltaTime;
}

void AEnemySpawner::SpawnEnemy()
{

	
	//Check if using the correct game instance; this is used to get the enemy class for spawning.
	if (const AMultiplayerGameMode* GameInstance = Cast<AMultiplayerGameMode>(GetWorld()->GetAuthGameMode()))
	{
		UE_LOG(LogTemp, Log, TEXT("Found GameInstance"));
		//Find a spawn location around the player. Then Make sure it is above ground.
		FVector SpawnPosition =
			PossibleSpawnLocations[FMath::RandRange(0, PossibleSpawnLocations.Num()-1)];
		SpawnPosition.Z += 50.0f;

		//Spawn Enemy and generate a stats struct for it using helper methods.
		AEnemyCharacter* Enemy = GetWorld()->SpawnActor<AEnemyCharacter>(GameInstance->GetEnemyClass(), SpawnPosition, FRotator::ZeroRotator, SpawnParameters);
		Enemy->SpawnDefaultController();
		FEnemyStats SpawnedEnemyStats;

		SpawnedEnemyStats.Aggression = GenerateAggression(PlayerKillsInLastMinute);
		SpawnedEnemyStats.SizeFactor = FMath::RandRange(0.5f, 2.5f);
		SpawnedEnemyStats.NoiseSensitivity = GenerateNoiseSensitivity(TimesPlayerDetected);
		SpawnedEnemyStats.InstaKillChance = GetInstaKillChance(PlayerSpecialKillsInLastMinute);
		
		//Scale enemy size and set the meshes new colour and glow. 
		float ScaleFactor = SpawnedEnemyStats.SizeFactor;
		if (Enemy)
		{
			Enemy->SetStats(SpawnedEnemyStats);
			Enemy->Multicast_SetColourAndGlow(GenerateColour(SpawnedEnemyStats.Aggression, SpawnedEnemyStats.NoiseSensitivity), SpawnedEnemyStats.NoiseSensitivity / 200);
			Enemy->Multicast_SetMeshSize(ScaleFactor);
			UE_LOG(LogTemp, Log, TEXT("Spawned"));
		}
		
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Could not find Game Intstance"));
	}
	
}

void AEnemySpawner::IncreaseKill(bool bIsSpecialKill)
{
	PlayerKillsInLastMinute++;
	if (bIsSpecialKill)
	{
		PlayerSpecialKillsInLastMinute++;
	}
}

void AEnemySpawner::IncreasePlayerDetected()
{
	TimesPlayerDetected++;
}

void AEnemySpawner::ResetKillsAndDetected()
{
	PlayerKillsInLastMinute = 0;
	PlayerSpecialKillsInLastMinute = 0;
	TimesPlayerDetected = 0;
}

//Semi-Random spawn amount determined by how many kills the player has gotten in the last minute.
int32 AEnemySpawner::GetEnemySpawnAmount(int ThreatLevel)
{
	int32 AmountToSpawn = 1;

	if (ThreatLevel <= 0.0f)
	{
		AmountToSpawn = 1;
	}

	else if (0.0f < ThreatLevel && ThreatLevel < 5.0f)
	{
		AmountToSpawn = FMath::RandRange(1, 3);
	}

	else if (5.0f <= ThreatLevel && ThreatLevel < 10.0f)
	{
		AmountToSpawn = FMath::RandRange(3, 6);
	}

	else if (ThreatLevel >= 10.0f)
	{
		AmountToSpawn = 8;
	}
	
	return AmountToSpawn;
}

FLinearColor AEnemySpawner::GenerateColour(float AggressionInput, float NoiseSensitivityInput)
{

	float NormalizedAggression = (AggressionInput - 10) / 90.0; // map 10-100 to 0-1
	float Red = 0.3 + 0.7 * NormalizedAggression;          // increase red
	float Green = 0.7 * (1.0 - NormalizedAggression);      // decrease green
	float Blue = 1.0 * (1.0 - NormalizedAggression);       // decrease blue



	float noiseFactor = 0.05f; // Adjust this factor for more/less variation
	Red += FMath::FRandRange(-noiseFactor, noiseFactor);   // Random variation for red
	Green += FMath::FRandRange(-noiseFactor, noiseFactor); // Random variation for green
	Blue += FMath::FRandRange(-noiseFactor, noiseFactor);  // Random variation for blue
	
	FLinearColor ReturnColour(Red, Green,Blue, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("Colour Seed: %f"), Red);
	return ReturnColour;
}

//Semi-Random Aggression determined by how many kills the player has gotten in the last minute.
float AEnemySpawner::GenerateAggression(float EnemiesKilledInput)
{
	float Aggression = 0.0f;
	if (EnemiesKilledInput <= 0)
	{
		Aggression = 10.0f;
		UE_LOG(LogTemp, Log, TEXT("Enemy Aggression: %F"), Aggression);
		return Aggression;
	}

	if (0 < EnemiesKilledInput && 2 >= EnemiesKilledInput)
	{
		Aggression = FMath::RandRange(11.0f, 30.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy Aggression: %F"), Aggression);
		return Aggression;
	}

	if (2 < EnemiesKilledInput && 5 >= EnemiesKilledInput)
	{
		Aggression = FMath::RandRange(31.0f, 60.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy Aggression: %F"), Aggression);
		return Aggression;
	}
	
	if (5 < EnemiesKilledInput && 10 >= EnemiesKilledInput)
	{
		Aggression = FMath::RandRange(61.0f, 85.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy Aggression: %F"), Aggression);
		return Aggression;
	}

	if (EnemiesKilledInput > 10)
	{
		Aggression =  FMath::RandRange(86.0f, 100.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy Aggression: %F"), Aggression);
		return Aggression;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get Aggression"));
		return Aggression;
	}
}
//Semi-Random Sensitivity To Noise (functionally not implemented as I am only concerned with the visuals) determined by how many times player has been detected before.
float AEnemySpawner::GenerateNoiseSensitivity(float DetectionInput)
{
	float NoiseSensitivity = 0.0f;
	if (DetectionInput <= 0)
	{
		NoiseSensitivity = 10.0f;
		UE_LOG(LogTemp, Log, TEXT("Enemy NoiseSensitivity: %F"), NoiseSensitivity);
		return NoiseSensitivity;
	}

	if (0 < DetectionInput && 2 >= DetectionInput)
	{
		NoiseSensitivity = FMath::RandRange(250.0f, 300.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy NoiseSensitivity: %F"), NoiseSensitivity);
		return NoiseSensitivity;
	}

	if (2 < DetectionInput && 5 >= DetectionInput)
	{
		NoiseSensitivity = FMath::RandRange(301.0f, 400.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy NoiseSensitivity: %F"), NoiseSensitivity);
		return NoiseSensitivity;
	}
	
	if (5 < DetectionInput && 10 >= DetectionInput)
	{
		NoiseSensitivity = FMath::RandRange(401.0f, 500.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy NoiseSensitivity: %F"), NoiseSensitivity);
		return NoiseSensitivity;
	}

	if (DetectionInput > 10)
	{
		NoiseSensitivity =  FMath::RandRange(501.0f, 600.0f);
		UE_LOG(LogTemp, Log, TEXT("Enemy NoiseSensitivity: %F"), NoiseSensitivity);
		return NoiseSensitivity;
	}
		UE_LOG(LogTemp, Error, TEXT("Failed to get NoiseSensitivity"));
		return NoiseSensitivity;
}

//Chance for player to insta-kill an enemy; increases the more special kills player gets, so that there is a higher chacne of killing the increased spawned enemies.
float AEnemySpawner::GetInstaKillChance(int SpecialKillsPerformed)
{
	if (SpecialKillsPerformed <= 0)
	{
		return 0.05f;
	}

	if (SpecialKillsPerformed == 1)
	{
		return 0.1f;
	}

	if (SpecialKillsPerformed == 2)
	{
		return 0.15f;
	}
	
	if (SpecialKillsPerformed == 3)
	{
		return 0.2f;
	}

	if (SpecialKillsPerformed >= 4)
	{
		return 0.3f;
	}

	return 0.05f;
}

void AEnemySpawner::PopulateSpawnLocations()
{
	PossibleSpawnLocations.Empty();
	if (const UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	{
		PossibleSpawnLocations = PathfindingSubsystem->GetWaypointPositions();
	}
}








