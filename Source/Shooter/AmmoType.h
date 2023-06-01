#pragma once

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm UMETA(FEnumDisplayName = "9mm"),
	EAT_AR UMETA(FEnumDisplayName = "Assoult Rifle"),

	EAT_MAX UMETA(FEnumDisplayName = "Deffaults Rifle")
};