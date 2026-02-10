// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSubsystem.generated.h"

/**
 * 
 */

class ISavableObject;
class ISavableActor;

struct FSaveSerializer
{
private:
	static FString GetPropertyTypeString(const FProperty* Property);

	static void SerializeProperty(FProperty* Property, void* ValuePtr, FPropertySaveData& OutSaveProp);

	static void DeserializeProperty(FProperty* Property, void* ValuePtr, const FPropertySaveData& SaveProp);

	static void AssignProperty(UObject* Object, const FPropertySaveData& SaveProp);

public:
	static TOptional<FObjectSaveData> SerializeActor(ISavableActor* SavableActor, FString Version);
	static void DeserializeActor(UObject* WorldContext, const FObjectSaveData& ObjectSaveData, FString Version, const TMap<FString, AActor*>& StaticSpawnedActorMap);
};


UCLASS()
class LOGICRAFTCOREUTILS_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	using VersionType = TTuple<int, int, int>;

	UPROPERTY(Transient)
	TObjectPtr<ULCUSaveGame> CurrentSave;
	
	TOptional<VersionType> ExtractVersion(const FString& Version);

	TMap<FString, AActor*> BuildStaticActorSpawnedMap() const;
	
public:

	UFUNCTION(BlueprintCallable)
	void SaveWorld(const FName& SlotName, const FString& Version);
	
	UFUNCTION(BlueprintCallable)
	void LoadWorld(const FName& SlotName, const FString& Version);	
};
