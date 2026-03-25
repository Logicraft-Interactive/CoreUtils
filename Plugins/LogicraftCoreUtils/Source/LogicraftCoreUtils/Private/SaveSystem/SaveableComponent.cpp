// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "SaveSystem/SaveableComponent.h"

USaveableComponent::USaveableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FString USaveableComponent::GetSaveVersion_Implementation()
{
	return TEXT("1.0.0");
}

void USaveableComponent::SetupSaveMigrateLogic_Implementation()
{
}

void USaveableComponent::AddMigrateDelegate(const FString& FromVersion, const FString& ToVersion, FSaveableComponentMigrateEventSignature Delegate)
{
	UClass* Class = GetClass();
	if (!Class)
	{
		return;
	}
		
	if (!MigratesDelegateMap.Find(Class))
	{
		MigratesDelegateMap.Add(Class);
	}
	
	MigratesDelegateMap[Class].Add(FromVersion, [Delegate, ToVersion](USaveableComponent* Comp, FString FromVer, const TArray<FPropertySaveData>& OldPropertyArray)
	{
		Delegate.ExecuteIfBound(Comp, FromVer, ToVersion, OldPropertyArray);
		return ToVersion;
	});
}

const USaveableComponent::FDelegateMapType& USaveableComponent::GetMigrateDelegateMap(const UObject* Object)
{
	if (const auto* Found = MigratesDelegateMap.Find(Object->GetClass()))
	{
		return *Found;
	}
	return EmptyMap;
}

void USaveableComponent::OnPreSave_Implementation()
{
}

void USaveableComponent::OnPostSave_Implementation()
{
}

void USaveableComponent::OnPreLoad_Implementation()
{
}

void USaveableComponent::OnPostLoad_Implementation()
{
}
