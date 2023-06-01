// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UShooterAnimInstance::UShooterAnimInstance() :
	Speed(0.f),
	bIsInAir(false),
	bIsAccelerating(false),
	MovementOffsetYaw(0.f),
	LastMovementOffsetYaw(0.f),
	bAiming(false),
	CharacterRotation(FRotator(0.f)),
	CharacterRotationLastFrame(FRotator(0.f)),
	TIPCharacterYaw(0.f),
	TIPCharacterYawLastFrame(0.f),
	YawDelta(0.f),
	RootYawOffset(0.f),
	Pitch(0),
	bReloading(false),
	OffsetState(EOffsetState::EOS_Hip),
	RecoilWeight(1.f),
	bCrouching(false),
	EquippedWeaponType(EWeaponType::EWT_MAX),
	bShouldUseFABRIK(false)
{
	
}

void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	// ako slučajni nije inicialitiran sa NativeInitializeAnimation() onda probaj ponovo
	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}

	if (ShooterCharacter)
	{
		bCrouching = ShooterCharacter->GetCrouching();
		bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;
		bEquipping = ShooterCharacter->GetCombatState() == ECombatState::ECS_Equipping;
		bShouldUseFABRIK = ShooterCharacter->GetCombatState() == ECombatState::ECS_Unoccupited || ShooterCharacter->GetCombatState() == ECombatState::ECS_FireTimerInProgress;
		
		// get LATERAL speed of character from velocity
		//FVector Velocity{ ShooterCharacter->GetVelocity() };
		FVector Velocity = ShooterCharacter->GetVelocity();
		Velocity.Z = 0;
		Speed = Velocity.Size();

		// is character in air?
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		// is character accelerating?
		if (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() >0)
		{
			bIsAccelerating = true;
		} else
		{
			bIsAccelerating = false;
		}

		FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());

		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

		if (ShooterCharacter->GetVelocity().Size() > 0)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}

		bAiming = ShooterCharacter->GetAiming();

		if (bReloading)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (ShooterCharacter->GetAiming()) {
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}

		// chack if shoootercharacter has a valid
		if (ShooterCharacter->GetEquippedWeapon())
		{
			EquippedWeaponType = ShooterCharacter->GetEquippedWeapon()->GetWeaponType();
		}
	}
	TurnInPlace();
	Lean(DeltaTime);
}

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::TurnInPlace()
{
	if (ShooterCharacter == nullptr)
	{
		return;
	}

	Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;

	

	if (Speed > 0 || bIsInAir)
	{
		// don't want to turn in place if character is mooving
		RootYawOffset = 0;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurveLastFrame = 0.f;
		RotationCurve  = 0.f;
	}
	else
	{
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		const float TIPYawDelta = TIPCharacterYaw - TIPCharacterYawLastFrame;

		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

		const float Turning = GetCurveValue(TEXT("Turning"));

		// 1.0 if turnung 0.0 if not
		if (Turning > 0)
		{
			bTurningInPlace = true;
			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));

			const float DeltaRotation = RotationCurve - RotationCurveLastFrame;

			// RootYawOffset > 0 -> turn left,  0 < -> turn right
			RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

			const float ABSRootYawOffset = FMath::Abs(RootYawOffset);
			if (ABSRootYawOffset >0)
			{
				const float YawExcess = ABSRootYawOffset - 90.f;
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		
		}
		else
		{
			bTurningInPlace = false;
		}

		
		//if (GEngine)
		//{
		//	GEngine->AddOnScreenDebugMessage(1, -1, FColor::Blue, FString::Printf(TEXT("TIPCharacterYaw: %f"), TIPCharacterYaw));
		//}
		//if (GEngine)
		//{
		//	GEngine->AddOnScreenDebugMessage(2, -1, FColor::Red, FString::Printf(TEXT("RootYawOffset: %f"), RootYawOffset));
		//}
	}

	// set the recoil weight
	if (bTurningInPlace)
	{
		if (bReloading || bEquipping)
		{
			RecoilWeight = 1.f;
		}
		else {
			RecoilWeight = 0.0f;
		}
	}
	else // not turning in place
	{
		if (bCrouching)
		{
			if (bReloading || bEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.1f;
			}
		}
		else
		{
			if (bAiming || bReloading || bEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.5f;
			}
		}
	}
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
	if (ShooterCharacter == nullptr) { return; }

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);

	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.f);
	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);

	//if (GEngine)
	//{
	//	GEngine->AddOnScreenDebugMessage(2, -1, FColor::Cyan, FString::Printf(TEXT("YawDelta: %f"), YawDelta));
	//}
}
