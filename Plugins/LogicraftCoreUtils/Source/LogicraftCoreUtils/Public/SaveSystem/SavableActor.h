// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "UObject/Interface.h"
#include "SavableActor.generated.h"

// This class does not need to be modified.
UINTERFACE()
class USavableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */


DECLARE_DYNAMIC_DELEGATE_ThreeParams(FMigrateEventSignature, FString, FromVersion, FString, ToVersion, const TArray<FPropertySaveData>&, OldPropertyArray);

class LOGICRAFTCOREUTILS_API ISavableActor
{
	GENERATED_BODY()

	bool bIsDynamicSpawned = false;
	TSubclassOf<AActor> DynamicSpawnClass = nullptr;
	FString UniqueID;
	using DelegateMapType = TMap<FString, TFunction<FString(FString, const TArray<FPropertySaveData>&)>>;
	DelegateMapType MigratesDelegateMap; 
	
public:
	void SetIsDynamicSpawned(TSubclassOf<UObject> SpawnActor, FGuid UID = FGuid::NewGuid());
	FString GetUniqueID() const;
	TSubclassOf<UObject> GetDynamicSpawnClass() const;
	bool GetIsDynamicSpawned() const;

	UFUNCTION(BlueprintNativeEvent, DisplayName = "Get Save Version")
	FString BP_GetVersion();
	FString BP_GetVersion_Implementation();
	virtual FString GetVersion() PURE_VIRTUAL(ThisClass::GetVersion(),return FString();)

	void AddMigrateDelegateNative(const FString& FromVersion, const FString& ToVersion,const FMigrateEventSignature& Delegate);
	
	template<typename Func>
	void AddMigrateDelegateLambda(const FString& FromVersion, const FString& ToVersion, Func&& Lambda)
	{
		MigratesDelegateMap.Add(FromVersion, [MigrateLogic = Forward<Func>(Lambda), ToVersion](auto From, auto PropertyArray)
		{
			MigrateLogic(From, ToVersion, PropertyArray);
			return ToVersion;
		});
	}

	const DelegateMapType& GetMigrateDelegateMap(); 
	
	
	virtual void OnPreLoad();
	virtual void OnPreSave();
	
	virtual void OnPostLoad();
	virtual void OnPostSave();
};

UCLASS()
class LOGICRAFTCOREUTILS_API USavableActorStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (DefaultToSelf = "Target"))
	static void AddMigrateDelegate(TScriptInterface<ISavableActor> Target, FString FromVersion, FString ToVersion, FMigrateEventSignature Delegate);
};
