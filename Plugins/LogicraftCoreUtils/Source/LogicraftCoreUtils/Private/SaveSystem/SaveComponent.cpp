// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "SaveSystem/SaveComponent.h"

USaveComponent::USaveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USaveComponent::SetIsDynamicSpawned(TSubclassOf<AActor> SpawnClass, FGuid UID)
{
	DynamicSpawnClass = SpawnClass;
	UniqueID = UID.ToString();
	bIsDynamicSpawned = true;
}

FString USaveComponent::GetUniqueID() const
{
	if (bIsDynamicSpawned)
	{
		return UniqueID;
	}
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetName() : FString();
}

TSubclassOf<AActor> USaveComponent::GetDynamicSpawnClass() const
{
	return DynamicSpawnClass;
}

bool USaveComponent::GetIsDynamicSpawned() const
{
	return bIsDynamicSpawned;
}

FString USaveComponent::GetSaveVersion_Implementation()
{
	return TEXT("1.0.0");
}

void USaveComponent::SetupSaveMigrateLogic_Implementation()
{
}

void USaveComponent::AddMigrateDelegate(const FString& FromVersion, const FString& ToVersion, FComponentMigrateEventSignature Delegate)
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
	
	MigratesDelegateMap[Class].Add(FromVersion, [Delegate, ToVersion](AActor* Actor, FString FromVer, const TArray<FPropertySaveData>& OldPropertyArray)
	{
		Delegate.ExecuteIfBound(Actor, FromVer, ToVersion, OldPropertyArray);
		return ToVersion;
	});
}

const USaveComponent::FDelegateMapType& USaveComponent::GetMigrateDelegateMap(const UObject* Object)
{
	if (const auto* Found = MigratesDelegateMap.Find(Object->GetClass()))
	{
		return *Found;
	}
	return EmptyMap;
}

void USaveComponent::OnPreSave_Implementation()
{
}

void USaveComponent::OnPostSave_Implementation()
{
}

void USaveComponent::OnPreLoad_Implementation()
{
}

void USaveComponent::OnPostLoad_Implementation()
{
}
