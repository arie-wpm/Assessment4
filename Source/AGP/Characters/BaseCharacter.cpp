// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "HealthComponent.h"
#include "PlayerCharacter.h"
#include "AGP/EnemySpawner.h"
#include "AGP/MultiplayerGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	BulletStartPosition = CreateDefaultSubobject<USceneComponent>("Bullet Start");
	BulletStartPosition->SetupAttachment(GetRootComponent());
	HealthComponent = CreateDefaultSubobject<UHealthComponent>("Health Component");
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (EnemySpawner == nullptr)
	{
		EnemySpawner = Cast<AEnemySpawner>(UGameplayStatics::GetActorOfClass(GetWorld(), AEnemySpawner::StaticClass()));
	}
	
}

void ABaseCharacter::Fire(const FVector& FireAtLocation)
{
	if (HasWeapon())
	{
		WeaponComponent->Fire(BulletStartPosition->GetComponentLocation(), FireAtLocation);
	}
}

void ABaseCharacter::Reload()
{
	if (HasWeapon())
	{
		WeaponComponent->Reload();
	}
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseCharacter, WeaponComponent);
}

void ABaseCharacter::OnDeath()
{
	// WE ONLY WANT TO HANDLE LOGIC IF IT IS ON THE SERVER
	if (GetLocalRole() != ROLE_Authority) return;
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(this))
	{
		// If it casts to a PlayerCharacter then we know that a player has died.
		if (AMultiplayerGameMode* GameMode = Cast<AMultiplayerGameMode>(GetWorld()->GetAuthGameMode()))
		{
			// Tell the GameMode to respawn this player.
			GameMode->RespawnPlayer(GetController());
		}
	}
	if(AEnemyCharacter* PlayerCharacter = Cast<AEnemyCharacter>(this))
	{
		PlayerCharacter->Destroy();
	}
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ABaseCharacter::HasWeapon()
{
	return (WeaponComponent != nullptr);
}

UHealthComponent* ABaseCharacter::GiveHealthComponent()
{
	return HealthComponent;
}

void ABaseCharacter::EquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		EquipWeaponImplementation(bEquipWeapon, WeaponStats);
		MulticastEquipWeapon(bEquipWeapon, WeaponStats);
	}
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

AEnemySpawner* ABaseCharacter::GetEnemySpawner()
{
	return EnemySpawner;
}

void ABaseCharacter::EquipWeaponImplementation(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	// Create or remove the weapon component depending on whether we are trying to equip a weapon and we
	// don't already have one. Or if we are trying to unequip a weapon and we do have one.
	if (bEquipWeapon && !HasWeapon())
	{
		WeaponComponent = NewObject<UWeaponComponent>(this);
		WeaponComponent->RegisterComponent();
	}
	else if (!bEquipWeapon && HasWeapon())
	{
		WeaponComponent->UnregisterComponent();
		WeaponComponent = nullptr;
	}

	// At this point we should have a WeaponComponent if we are trying to equip a weapon.
	if (HasWeapon())
	{
		// Set the weapons stats to the given weapon stats.
		UE_LOG(LogTemp, Display, TEXT("Equipping weapon: \n%s"), *WeaponStats.ToString())
		WeaponComponent->SetWeaponStats(WeaponStats);
	}

	if (bEquipWeapon)
	{
		UE_LOG(LogTemp, Display, TEXT("Player has equipped weapon."))
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Player has unequipped weapon."))
	}
}

void ABaseCharacter::MulticastEquipWeapon_Implementation(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	//EquipWeaponImplementation(bEquipWeapon, WeaponStats);
	EquipWeaponGraphical(bEquipWeapon);
}

int ABaseCharacter::GetEnemiesKilledInLastMinute() const
{
	return EnemiesKilledInLastMinute;
}

