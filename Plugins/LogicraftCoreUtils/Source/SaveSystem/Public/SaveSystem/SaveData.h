// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Object.h"
#include "SaveData.generated.h"

/**
 * FPropertySaveData
 *
 * Serialized representation of a single UPROPERTY marked with the SaveGame flag.
 * Stores the property name, its C++ type as a string, and the raw binary data
 * produced by Unreal's structured archive serialization.
 */
USTRUCT(Blueprintable, BlueprintType)
struct FPropertySaveData
{
	GENERATED_BODY()

	/** Name of the serialized property (must match the UPROPERTY FName on the owning class). */
	UPROPERTY(SaveGame)
	FName PropertyName;

	/** C++ type string used to detect type mismatches when loading (e.g. "int32", "FVector"). */
	UPROPERTY(SaveGame)
	FString PropertyType;
	
	/** Raw binary payload produced by FStructuredArchive serialization. */
	UPROPERTY(SaveGame)
	TArray<uint8> PropertyData;	
};

/**
 * FComponentSaveData
 *
 * Saved state of a single actor component that derives from USaveableComponent.
 * Groups all serialized properties for one component together with its
 * version information for migration support.
 */
USTRUCT()
struct FComponentSaveData
{
	GENERATED_BODY()

	/** Unique identifier of the component within its owning actor (UActorComponent::GetName). */
	UPROPERTY(SaveGame)
	FString ComponentID;
	
	/** Version string at the time of serialization, used to trigger migration logic on load. */
	UPROPERTY(SaveGame)
	FString SaveVersion {"1.0.0"};
	
	/** Array of serialized properties for this component. */
	UPROPERTY(SaveGame)
	TArray<FPropertySaveData> Properties;
	
	/** Expected property count, used to detect possible data corruption. */
	UPROPERTY(SaveGame)
	int32 PropertiesCount{0};
};

/**
 * FObjectSaveData
 *
 * Complete saved state of an actor that has a USaveComponent attached.
 * Contains identification, spawn information, version, serialized properties,
 * and the saved state of all its USaveableComponent sub-components.
 */
USTRUCT()
struct FObjectSaveData
{
	GENERATED_BODY()

	/** Unique identifier of the actor (FGuid string for dynamic actors, actor name for static ones). */
	UPROPERTY(SaveGame)
	FString ObjectID;

	/** Class used to spawn or locate the actor during deserialization. */
	UPROPERTY(SaveGame)
	TSubclassOf<UObject> ObjectClass;

	/** Version string at the time of serialization, compared against the current version on load. */
	UPROPERTY(SaveGame)
	FString SaveVersion {"1.0.0"};

	/** World transform to restore when re-spawning a dynamic actor. */
	UPROPERTY(SaveGame)
	FTransform SpawnTransform;
	
	/** If true, the actor was runtime-spawned and will be re-created on load. Otherwise it is found in the level. */
	UPROPERTY(SaveGame)
	bool bIsDynamicSpawned {false};

	/** Array of serialized properties for this actor. */
	UPROPERTY(SaveGame)
	TArray<FPropertySaveData> Properties;

	/** Expected property count, used to detect possible data corruption. */
	UPROPERTY(SaveGame)
	int32 PropertiesCount{0};
	
	/** Array of saved component data for all USaveableComponent sub-components on this actor. */
	UPROPERTY(SaveGame)
	TArray<FComponentSaveData> Components;

	/** Expected component count, used to detect possible data corruption. */
	UPROPERTY(SaveGame)
	int32 ComponentsCount{0};
};


/**
 * ULCUSaveGame
 *
 * Root USaveGame object that holds the entire world save state.
 * Serialized to / loaded from a named slot via UGameplayStatics.
 */
UCLASS()
class SAVESYSTEM_API ULCUSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	/** Global version string written at save time (semantic versioning: "Major.Minor.Patch"). */
	UPROPERTY(SaveGame)
	FString GlobalSaveVersion = "1.0.0";

	/** Timestamp of when the save was created. */
	UPROPERTY(SaveGame)
	FDateTime SaveTimeStamp;

	/** Slot name this save was written to. */
	UPROPERTY(SaveGame)
	FName SaveSlotName;

	/** All serialized actor data contained in this save. */
	UPROPERTY(SaveGame)
	TArray<FObjectSaveData> ObjectsData;	
};