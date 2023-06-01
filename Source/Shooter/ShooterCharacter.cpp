// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"

#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Item.h"
#include "Weapon.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/WidgetComponent.h"
#include <Shooter/Ammo.h>

#include "BulletHitInterface.h"
#include "Enemy.h"
#include "EnemyController.h"
#include "Shooter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"


// Sets default values
AShooterCharacter::AShooterCharacter()
{
	// base rates for turning / look up
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;
	bAiming = false;

	// turn rates for aiming / not aiming
	HipTurnRate = 90.f;
	HipLookUpRate = 90.f;
	AimingLookUpRate = 20.f;
	AimingTurnRate = 20.f;

	// mouse look sensitivity scale
	MouseHipTurnRate = 1.f;
	MouseHipLookUpRate = 1.f;
	MouseAimingTurnRate = 0.6f;
	MouseAimingLookUpRate = 0.6f;

	// camera FOV values
	CameraDefaultFOV = 0.0f; // set later in BeginPlay()
	CameraZoomedFOV = 25.0f;
	CameraCurrentFOV = 0.0f; // set later in BeginPlay()
	ZoomInterpSpeed = 20.0f;

	// crosshair spread factors
	CrosshairSpreadMultiplier = 0.f;
	CrosshairVelocityFactor = 0.f;
	CrosshairInAirFactor = 0.f;
	CrosshairAimFator = 0.f;
	CrosshairShootingFactor = 0.f;

	// automatic fire variables
	//AutomaticFireRate = 0.1f;
	bShouldFire = true;
	bFireButtonPressed = false;

	// item trace variables
	bShouldTraceItems = false;
	OverlappedItemCount = 0;

	// camera interp location variables
	CameraInterpDistance = 250.f;
	CameraInterpElevation = 65.f;

	// bullet fire timer variables
	ShootTimeDuration = 0.05f;
	bFiringBullet = false;
	bFireButtonPressed = false;
	// tarting ammo ammounts
	Starting9mmAmmo = 32;
	StartingARAmmo = 120;
	// combat variables
	CombatState = ECombatState::ECS_Unoccupited;
	bCrouching = false;
	BaseMovementSpeed = 650.f;
	CrouchMovementSpeed = 300.f;
	StandingCapsuleHalfHeight = 88.f;
	CrouchingCapsuleHalfHeight = 44.f;
	BaseGroundFriction = 2.f;
	CrouchingGroundFriction = 100.f;
	bAimingButtonPressed = false;
	// pickip sound timer properties
	bShouldPlayPickupSound = true;
	bShouldPlayEquipSound = true;
	PickupSoundResetTime = 0.2f;
	EquipSoundResetTime = 0.2f;
	// icon animation properties
	HighlightedSlot = -1;
	Health = 100.f;
	MaxHealth = 100.f;
	StunChance = 0.25f;
	bDead = false;

	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//UE_LOG(LogTemp, Warning, TEXT("AShooterCharacter!"));

	// create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.0f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	// create follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // ne rotira relativno na šuštu

	// Don't rotate when the controler rotates, kontorler nek utjeće samo na kameru
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// config character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// create hand scene component
	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	// create interpolation components
	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponInterpComp"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpComp1"));
	InterpComp1->SetupAttachment(GetFollowCamera());

	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpComp2"));
	InterpComp2->SetupAttachment(GetFollowCamera());

	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpComp3"));
	InterpComp3->SetupAttachment(GetFollowCamera());

	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpComp4"));
	InterpComp4->SetupAttachment(GetFollowCamera());

	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpComp5"));
	InterpComp5->SetupAttachment(GetFollowCamera());

	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpComp6"));
	InterpComp6->SetupAttachment(GetFollowCamera());

}

float AShooterCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("TakeDamage"));
	UE_LOG(LogTemp, Warning, TEXT("Damage: %f"), DamageAmount);
	UE_LOG(LogTemp, Warning, TEXT("Health: %f"), Health);
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();

		AEnemyController* EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController)
		{
			EnemyController->GetBlacboardCompomponent()->SetValueAsBool(FName("CharacterDead"), true);
		}
	}
	else
	{
		Health -= DamageAmount;
	}

	return  DamageAmount;
}

void AShooterCharacter::Die()
{
	UE_LOG(LogTemp, Warning, TEXT("Die"));
	bDead = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimInstance && DeathMontage"));
		AnimInstance->Montage_Play(DeathMontage);
		AnimInstance->Montage_JumpToSection(FName("Death_A"), DeathMontage);
	}
}

void AShooterCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		DisableInput(PC);
	}

}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(LogTemp, Warning, TEXT("BeginPlay!"));

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	// spawn and attach default weapon
	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);
	EquippedWeapon->SetSlotIndex(0);
	EquippedWeapon->DisableCustomDepth();
	EquippedWeapon->DisableGlowMaterial();
	EquippedWeapon->SetCharacter(this);
	
	InitializeAmmoMap();
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

	// create FinterpLocation structs
	InitializeInterpLocations();

}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		// find forward
		const FRotator Rotation = Controller->GetControlRotation();
		//const FRotator YawRotation{ 0, Rotation.Yaw, 0  };
		const FRotator YawRotation = FRotator(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		// find right
		const FRotator Rotation{ Controller->GetControlRotation() };
		//const FRotator YawRotation{ 0, Rotation.Yaw, 0 };
		const FRotator YawRotation = FRotator(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// izračunaj deltu za jedan frame
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor;
	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipLookUpRate;
	}
	AddControllerYawInput(Value * TurnScaleFactor);

}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor;
	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value * LookUpScaleFactor);
}

void AShooterCharacter::PlayFireSound()
{
	if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AShooterCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

		//UE_LOG(LogTemp, Warning, TEXT("BEFORE MuzzleFlash"));
		if (EquippedWeapon->GetMuzzleFlash())
		{
			//UE_LOG(LogTemp, Warning, TEXT("MuzzleFlash"));
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
		}

		FHitResult BeamHitResoult;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamHitResoult);

		if (bBeamEnd)
		{
			if (BeamHitResoult.Actor.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("Actor Name: %s"),*BeamHitResoult.GetActor()->GetName());
				
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResoult.Actor.Get());

				if (BulletHitInterface)
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResoult, this, GetController());
				}

				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResoult.Actor.Get());
				if (HitEnemy)
				{
					int32 Damage = 0;
					if (BeamHitResoult.BoneName.ToString() == HitEnemy->GetHeadBone())
					{
						// headshot
						Damage = EquippedWeapon->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(BeamHitResoult.Actor.Get(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass()
						);

						HitEnemy->ShowHitNumber(Damage, BeamHitResoult.Location, true);
					}
					else
					{
						// bodyshots
						Damage = EquippedWeapon->GetDamage();
						UGameplayStatics::ApplyDamage(BeamHitResoult.Actor.Get(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass()
						);

						HitEnemy->ShowHitNumber(Damage, BeamHitResoult.Location, false);
					}

					
					
					
					UE_LOG(LogTemp, Warning, TEXT("Hit Component: %s"), *BeamHitResoult.BoneName.ToString());
				}
			}
			else
			{
				if (ImapactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImapactParticles, BeamHitResoult.Location);
				}
			}

			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamHitResoult.Location);
			}

		}

	}
}

void AShooterCharacter::PlayGunfireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		//UE_LOG(LogTemp, Warning, TEXT("AnimInstance && HipFireMontage"));
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupited)
	{
		return;
	}

	
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	// do we have ammo of correct type?
	if (CarryingAmmo() && !EquippedWeapon->ClipIsFull()) 
	{

		if (bAiming)
		{
			StopAiming();
		}
	
		CombatState = ECombatState::ECS_Reloading;
		
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
	
}

bool AShooterCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}

	EAmmoType AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}

	return false;
}

void AShooterCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (HandSceneComponent == nullptr)
	{
		return;
	}

	int32 ClipBoneIndex = EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName());
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttacmentRules = FAttachmentTransformRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttacmentRules, FName(TEXT("hand_l")));
	HandSceneComponent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

void AShooterCharacter::ReleaseClip()
{
	EquippedWeapon->SetMovingClip(false);
}

void AShooterCharacter::CrouchButtonPressed()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}

	if (bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AShooterCharacter::Jump()
{
	if (bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		ACharacter::Jump();
	}
}

void AShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight;
	if (bCrouching)
	{
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else
	{
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}

	const float InterpHalfHeight = FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), TargetCapsuleHalfHeight, DeltaTime, 20.f);

	// negative value if crouching
	const float DeltaCapsuleHalfHeight = InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector MeshOffset = FVector(0.f, 0.f, -DeltaCapsuleHalfHeight);
	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AShooterCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (CombatState != ECombatState::ECS_Unoccupited)
	{
		return;
	}

	if (WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunfireMontage();

		EquippedWeapon->DecrementAmmo();
		StartFireTimer();

		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol)
		{
			// start moving slide timer
			EquippedWeapon->StartSlideTimer();
		}
		
		//StartCrosshairBulletFire(); // višak???
	}
		
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzlleSocketLocation, FHitResult& OutHitResoult)
{
	FVector OutBeamLocation;
	
	// check for crosshair hit
	FHitResult CrosshairHitResoult;
	bool bCrosshairHit = TraceUnderCrosHairs(CrosshairHitResoult, OutBeamLocation);

	if (bCrosshairHit)
	{
		// tentative beam location
		OutBeamLocation = CrosshairHitResoult.Location;
	}
	else // co crosshait hit
	{
		// outbeamlocation is the End location for the line trace
	}

	// performn second trace, this from cijev od puške

	const FVector WeaponTraceStart = MuzlleSocketLocation;
	const FVector StartToEnd = OutBeamLocation - MuzlleSocketLocation;
	const FVector WeaponTraceEnd = MuzlleSocketLocation + (StartToEnd * 1.25f);
	GetWorld()->LineTraceSingleByChannel(OutHitResoult, WeaponTraceStart, WeaponTraceEnd, ECC_Visibility);

	if (!OutHitResoult.bBlockingHit)
	{
		OutHitResoult.Location = OutBeamLocation;
		return  false;
	}

	return true;


}

void AShooterCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping && CombatState != ECombatState::ECS_Stunned)
	{
		Aim();
	}
}

void AShooterCharacter::StopAiming()
{
	bAiming = false;
	if (!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AShooterCharacter::PickupAmmo(AAmmo* Ammo)
{
	// check if ammo map contains ammotype
	if (AmmoMap.Find(Ammo->GetAmmoType()))
	{
		// get ammount of ammo in ammo map
		int32 AmmoCount = AmmoMap[Ammo->GetAmmoType()];
		AmmoCount += Ammo->GetItemCount();
		// set amount of ammo
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}

	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		// check if gun is empty
		if (EquippedWeapon->GetAmmo() == 0)
		{
			ReloadWeapon();
		}
	}

	Ammo->Destroy();
}

void AShooterCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation = FInterpLocation { WeaponInterpComp, 0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterLoc1 { InterpComp1, 0 };
	InterpLocations.Add(InterLoc1);

	FInterpLocation InterLoc2{ InterpComp2, 0 };
	InterpLocations.Add(InterLoc2);

	FInterpLocation InterLoc3{ InterpComp3, 0 };
	InterpLocations.Add(InterLoc3);

	FInterpLocation InterLoc4{ InterpComp4, 0 };
	InterpLocations.Add(InterLoc4);

	FInterpLocation InterLoc5{ InterpComp5, 0 };
	InterpLocations.Add(InterLoc5);

	FInterpLocation InterLoc6{ InterpComp6, 0 };
	InterpLocations.Add(InterLoc6);
}

void AShooterCharacter::FKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex()==0)
	{
		return;
	}
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::OneKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 1)
	{
		return;
	}
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::TwoKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 2)
	{
		return;
	}
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AShooterCharacter::ThreeKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 3)
	{
		return;
	}
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AShooterCharacter::FourKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 4)
	{
		return;
	}
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AShooterCharacter::FiveKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 5)
	{
		return;
	}
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AShooterCharacter::ExchangeInventoryItems(int32 CurrentItemindex, int32 NewItemIndex)
{

	const bool bCanExchangeItems = (CurrentItemindex != NewItemIndex) && 
								   (NewItemIndex < Inventory.Num())   && 
								   (CombatState == ECombatState::ECS_Unoccupited || CombatState == ECombatState::ECS_Equipping);
	
	if (bCanExchangeItems)
	{
		if (bAiming)
		{
			StopAiming();
		}
		
		AWeapon* OldEquippedWeapon = EquippedWeapon;
		AWeapon* NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
	
		EquipWeapon(NewWeapon);
		OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);

		CombatState = ECombatState::ECS_Equipping;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.0f);
			AnimInstance->Montage_JumpToSection("Equip");
		}

		NewWeapon->PlayEquipSound(true);
	}
}

int32 AShooterCharacter::GetEmptyInvetorySlot()
{
	for (int32 i = 0 ; i < Inventory.Num(); i++)
	{
		if (Inventory[i] == nullptr)
		{
			return i;
		}
	}

	if (Inventory.Num() < INVENTORY_CAPACITY)
	{
		return Inventory.Num();
	}

	return -1; // inventoy is full
}

void AShooterCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot = GetEmptyInvetorySlot();
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
	
}

EPhysicalSurface AShooterCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start = GetActorLocation();
	const FVector End = Start + FVector(0.f, 0.f, -400.f);
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;
	
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QueryParams);
	
	//UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *HitResult.Actor->GetName());

	//EPhysicalSurface HitSurface = HitResult.PhysMaterial.Get()->SurfaceType;

	//if (HitSurface)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("HHITSURFACE"));
	//}
	//

	//if (HitSurface == EPhysicalSurface::SurfaceType1) // grass
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Hit SurfaceType1 surface type"));
	//}
	//if (HitSurface == EPhysicalSurface::SurfaceType2) // grass
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Hit SurfaceType2 surface type"));
	//}
	//if (HitSurface == EPhysicalSurface::SurfaceType3) // grass
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Hit SurfaceType3 surface type"));
	//}
	//if (HitSurface == EPhysicalSurface::SurfaceType4) // grass
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Hit SurfaceType4 surface type"));
	//}
	//if (HitSurface == EPhysicalSurface::SurfaceType_Default) // grass
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Hit SurfaceType_Default surface type"));
	//}

	return  UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
	
}

void AShooterCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupited;

	if (bAimingButtonPressed)
	{
		Aim();
	}
}

void AShooterCharacter::UnHighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

void AShooterCharacter::Stun()
{
	if (Health <= 0.f)
	{
		return;
	}

	CombatState = ECombatState::ECS_Stunned;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

int32 AShooterCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;

	for (int i = 1; i < InterpLocations.Num(); ++i)
	{
		if (InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}

	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if (Amount < -1 || Amount >1)
	{
		return;
	}

	if (InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AShooterCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(PickupSoundTimer, this, &AShooterCharacter::ResetPickupSoundTimer, PickupSoundResetTime);
}

void AShooterCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AShooterCharacter::ResetEquipSoundTimer, PickupSoundResetTime);
}

void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	if (bAiming)
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{

	FVector2D WalkSpeedRange = FVector2D(0.f, 600.f);
	FVector2D VelocityMultiplierRange = FVector2D(0.f, 1.f);
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	// calculate crosshair in air factor
	if (GetCharacterMovement()->IsFalling())
	{
		// spread crosshair
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{	// character on the ground - shrink crosshair
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	//calc crosshair aim factor 
	if (bAiming)
	{
		CrosshairAimFator = FMath::FInterpTo(CrosshairAimFator, 0.6f, DeltaTime, 30.f);
	}
	else // not aiming
	{
		CrosshairAimFator = FMath::FInterpTo(CrosshairAimFator, 0.f, DeltaTime, 30.f);
	}

	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	}

	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFator + CrosshairShootingFactor;
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	CombatState = ECombatState::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, EquippedWeapon->GetAutoFireRate());


}

void AShooterCharacter::AutoFireReset()
{
	if (CombatState == ECombatState::ECS_Stunned)
	{
		return;
	}

	CombatState = ECombatState::ECS_Unoccupited;

	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed && EquippedWeapon->GetAutomatic())
		{
			FireWeapon();
		}
	} else
	{
		// reload weapon
		ReloadButtonPressed();
	}
}

bool AShooterCharacter::TraceUnderCrosHairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	// get viewport size
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// get screen space of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// get world position and direction of crisshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		// trace from crosshair wortld location outward
		const FVector Start = CrosshairWorldPosition;
		const FVector End = Start + CrosshairWorldDirection * 50000.f;

		OutHitLocation = End;

		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

void AShooterCharacter::TraceForItems()
{
	if (bShouldTraceItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation; // dummy, čisto da zadovoljimo parametre funkcije
		TraceUnderCrosHairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.Actor);
			
			const AWeapon* TraceGitWeapon = Cast<AWeapon>(TraceHitItem);
			
			if (TraceGitWeapon)
			{
				if (HighlightedSlot == -1)
				{
					// not currently highlighting slot - highlight one
					HighlightInventorySlot();
				}
			} else
			{
				// is a slot being highlighted?
				if (HighlightedSlot != -1)
				{
					UnHighlightInventorySlot();
				}
			}

			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}
			
			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				// show item's pickup widget
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();

				if (Inventory.Num() >= INVENTORY_CAPACITY)
				{
					// inventory full
					TraceHitItem->SetCharacterInventoryFull(true);
				}
				else
				{
					// inventory has room
					TraceHitItem->SetCharacterInventoryFull(false);
				}
			}

			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					// hitting different Aitem or Aitem is null
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}

			// store reference to HitItem
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame) {
		// no longer overlapping any items
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}

	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwaping)
{
	if (WeaponToEquip)
	{				
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		if (EquippedWeapon == nullptr)
		{
			// -1 = no equped weapon
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		} else if (!bSwaping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}
		
		// set equip weapon to newly spawned
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules = FDetachmentTransformRules(EDetachmentRule::KeepWorld,true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AShooterCharacter::SelectButtonPressed()
{
	if (CombatState != ECombatState::ECS_Unoccupited)
	{
		return;
	}
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this, true);
		TraceHitItem = nullptr;
	}
	
}

void AShooterCharacter::SelectButtonReleased()
{
	
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	if (Inventory.Num()-1 >= EquippedWeapon->GetSlotIndex() )
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}
	
	DropWeapon();
	EquipWeapon(WeaponToSwap, true);

	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		return  false;
	}

	return EquippedWeapon->GetAmmo() > 0; // true
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// set current camera FOV
	CameraInterpZoom(DeltaTime);

	// set turn rate and look up rate base d on aiming
	SetLookRates();

	// promjena pozicije ciljnika s brzinom kretanja
	CalculateCrosshairSpread(DeltaTime);

	// check overapped item count i onda trace for items
	TraceForItems();

	// interpolate the capsule half height
	InterpCapsuleHalfHeight(DeltaTime);
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);

	PlayerInputComponent->BindAction("FKey", IE_Pressed, this, &AShooterCharacter::FKeyPressed);
	PlayerInputComponent->BindAction("1Key", IE_Pressed, this, &AShooterCharacter::OneKeyPressed);
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this, &AShooterCharacter::TwoKeyPressed);
	PlayerInputComponent->BindAction("3Key", IE_Pressed, this, &AShooterCharacter::ThreeKeyPressed);
	PlayerInputComponent->BindAction("4Key", IE_Pressed, this, &AShooterCharacter::FourKeyPressed);
	PlayerInputComponent->BindAction("5Key", IE_Pressed, this, &AShooterCharacter::FiveKeyPressed);

}

void AShooterCharacter::FinishReloading()
{
	if (CombatState == ECombatState::ECS_Stunned)
	{
		return;
	}

	CombatState = ECombatState::ECS_Unoccupited;

	if (bAimingButtonPressed)
	{
		Aim();
	}

	if (EquippedWeapon == nullptr)
	{
		return;
	}

	const EAmmoType AmmoType = EquippedWeapon->GetAmmoType();
	
	// update ammo map
	if (AmmoMap.Contains(AmmoType))
	{
		// ammount of ammo the character is carryng
		int32 CarriedAmmo = AmmoMap[AmmoType];

		// space left in the magazine
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo)
		{
			// reload the magazine with all the ammo we are carrying
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		} else
		{
			// fill the magazine
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AShooterCharacter::FinishEquipping()
{
	if (CombatState == ECombatState::ECS_Stunned)
	{
		return;
	}

	CombatState = ECombatState::ECS_Unoccupited;
	if (bAimingButtonPressed)
	{
		Aim();
	}
}

void AShooterCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}

void AShooterCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	//UE_LOG(LogTemp, Warning, TEXT("spread %f"), CrosshairSpreadMultiplier);
	return  CrosshairSpreadMultiplier;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceItems = true;
	}
}

//FVector AShooterCharacter::GetCameraInterpLocation()
//{
//	const FVector CameraWorldLocation = FollowCamera->GetComponentLocation();
//	const FVector CameraForward = FollowCamera->GetForwardVector();
//
//	return CameraWorldLocation + (CameraForward * CameraInterpDistance) + FVector(0.f, 0.f, CameraInterpElevation);
//}

void AShooterCharacter::GetPickupItem(AItem* Item)
{
	Item->PlayEquipSound();
	
	AWeapon* Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		if (Inventory.Num() < INVENTORY_CAPACITY)
		{
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else // inventory is full - swap with EquippedWeapon
		{
			SwapWeapon(Weapon);
		}
		
	}

	AAmmo* Ammo = Cast<AAmmo>(Item);
	if (Ammo)
	{
		PickupAmmo(Ammo);
	}
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index)
{
	if (Index <= InterpLocations.Num())
	{
		return  InterpLocations[Index];
	}
	return FInterpLocation();
}


