// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

AWeapon::AWeapon() :
	ThrowWeaponTime(0.7f),
	bIsFalling(false),
	Ammo(30),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9mm),
	MagazieCapacity(30),
	ReloadMontageSection(FName(TEXT("Reload SMG"))),
	ClipBoneName(TEXT("smg_clip")),
	SlideDisplacement(0.f),
	SlideDisplacementTime(0.2f),
	bMovingSlide(false),
	MaxSlideDisplacement(4.f),
	MaxRecoilRotation(20.f),
	bAutomatic(true)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// keep weapon up right
	if (GetItemState() == EItemState::EIS_Falling && bIsFalling)
	{
		const FRotator MeshRotation = FRotator(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// update slide on pistol
	UpdateSlideDisplacement();
}

void AWeapon::ThrowWeapon()
{
	FRotator MeshRotation = FRotator(0.0f, GetItemMesh()->GetComponentRotation().Yaw, 0.0f);
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector MeshForward = FVector(GetItemMesh()->GetForwardVector());
	const FVector MeshRight = FVector(GetItemMesh()->GetRightVector());

	// smjer u kojem bacamo oružje
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

	float RandomRotation =  FMath::FRandRange(0.f,30.f);
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0, 0, 1));

	ImpulseDirection *= 20000;
	GetItemMesh()->AddImpulse(ImpulseDirection);

	bIsFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);

	EnableGlowMaterial();
}

void AWeapon::DecrementAmmo()
{
	if (Ammo-1 <= 0)
	{
		Ammo = 0;
	} else
	{
		--Ammo;
	}
}

void AWeapon::StartSlideTimer()
{
	bMovingSlide = true;
	GetWorldTimerManager().SetTimer(SlideTimer, this, &AWeapon::FinishMovingSlide, SlideDisplacementTime);
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazieCapacity, TEXT("Attempted to reload with more than magatine capacity"));
	Ammo += Amount;
}

bool AWeapon::ClipIsFull()
{
	return  Ammo >= MagazieCapacity;
}

void AWeapon::StopFalling()
{
	bIsFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	StartPulseTimer();
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	const FString WeaponTablePath(TEXT("DataTable'/Game/_Game/DataTable/WeaponDatatable.WeaponDatatable'"));
	UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));

	FWeaponDataTable* WeaponDataRow = nullptr;
	
	switch (WeaponType) {
	case EWeaponType::EWT_SubmachineGun:
		WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
		break;
	case EWeaponType::EWT_AssaultRifle:
		WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssoultRifle"), TEXT(""));
		break;
	case EWeaponType::EWT_Pistol:
		WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
		break;

	default: ;
	}

	if (WeaponDataRow)
	{
		AmmoType = WeaponDataRow->AmmoType;
		Ammo = WeaponDataRow->WeaponAmmo;
		MagazieCapacity = WeaponDataRow->MagazineCapacity;
		SetPickupSound(WeaponDataRow->PickupSound);
		SetEquipSound(WeaponDataRow->EquipSound);
		GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
		SetItemName(WeaponDataRow->ItemName);
		SetIconItem(WeaponDataRow->InventoryIcon);
		SetAmmoIcon(WeaponDataRow->AmmoIcon);

		SetMaterialInstance(WeaponDataRow->MaterialInstance);
		PreviousMaterialIndex = GetMaterialIndex();
		GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
		SetMaterialIndex(WeaponDataRow->MaterialIndex);
		SetClipBoneName(WeaponDataRow->ClipBoneName);
		SetReloadMontageSection(WeaponDataRow->ReloadMontageSection);
		GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);
		CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
		CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
		CrosshairsTop = WeaponDataRow->CrosshairsTop;
		CrosshairsRight = WeaponDataRow->CrosshairsRight;
		CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
		AutoFireRate = WeaponDataRow->AutoFireRate;
		MuzzleFlash = WeaponDataRow->MuzzleFlash;
		FireSound = WeaponDataRow->FireSound;
		BoneToHide = WeaponDataRow->BoneToHide;
		bAutomatic = WeaponDataRow->bAutomatic;
		Damage = WeaponDataRow->Damage;
		HeadShotDamage = WeaponDataRow->HeadShotDamage;

	}

	if (GetMaterialInstance())
	{
		SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
		GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
		GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
		EnableGlowMaterial();
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (BoneToHide != FName(""))
	{
		GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
	}
}

void AWeapon::FinishMovingSlide()
{
	bMovingSlide = false;
}

void AWeapon::UpdateSlideDisplacement()
{
	if (SlideDisplacementCurve && bMovingSlide)
	{
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(SlideTimer);
		const float CurveValue = SlideDisplacementCurve->GetFloatValue(ElapsedTime);
		SlideDisplacement = CurveValue * MaxSlideDisplacement;
		RecoilRotation = CurveValue * MaxRecoilRotation;
	}
}
