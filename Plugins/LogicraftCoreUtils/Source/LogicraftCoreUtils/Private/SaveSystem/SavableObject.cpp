// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "SaveSystem/SavableObject.h"

void ISavableObject::AddMigrateDelegateNative(const FString& FromVersion, const FString& ToVersion,
	const FObjectMigrateEventSignature& Delegate)
{
	UClass* Class = Cast<UObject>(this)->GetClass();
	if (!Class)
	{
		return;
	}
		
	if (!MigratesDelegateMap.Find(Class))
	{
		MigratesDelegateMap.Add(Class);
	}
	
	MigratesDelegateMap[Class].Add(FromVersion, [=](auto Object, auto FromVersion_, auto OldPropertyArray)
	{
		Delegate.ExecuteIfBound(Object, FromVersion_, ToVersion, OldPropertyArray);
		return ToVersion;
	});
}

const ISavableObject::FDelegateMapType& ISavableObject::GetMigrateDelegateMap(UObject* This)
{
	if (MigratesDelegateMap.Find(This->GetClass()))
	{
		return MigratesDelegateMap[This->GetClass()];
	}
	return EmptyMap;
}

// Add default functionality here for any ISavableObject functions that are not pure virtual.
void ISavableObject::OnPreLoad()
{
}

void ISavableObject::OnPreSave()
{
}

void ISavableObject::OnPostLoad()
{
}

void ISavableObject::OnPostSave()
{
}

void USavableUObjectStatics::AddMigrateDelegate(TScriptInterface<ISavableObject> Target, FString FromVersion,
	FString ToVersion, FObjectMigrateEventSignature Delegate)
{
	if (Target.GetInterface())
	{
		Target->AddMigrateDelegateNative(FromVersion, ToVersion, Delegate);
	}
}
