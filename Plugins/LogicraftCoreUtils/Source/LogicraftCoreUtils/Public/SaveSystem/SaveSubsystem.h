// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSubsystem.generated.h"

class ISavableObject;
class ISavableActor;

/**
 * FSaveSerializer
 *
 * Static utility struct responsible for converting actors and their components
 * to and from FObjectSaveData. Handles property iteration, binary serialization
 * via FStructuredArchive, type checking, and version migration dispatch.
 */
struct FSaveSerializer
{
private:
	/** @brief Returns the C++ type string for a given FProperty (used for type-mismatch detection). */
	static FString GetPropertyTypeString(const FProperty* Property);

	/**
	 * @brief Serializes a single property value into binary data.
	 * @param Property The property to serialize.
	 * @param ValuePtr Pointer to the property's value in memory.
	 * @param OutSaveProp Output struct that receives the binary payload.
	 */
	static void SerializeProperty(FProperty* Property, void* ValuePtr, FPropertySaveData& OutSaveProp);

	/**
	 * @brief Deserializes binary data back into a property value.
	 * @param Property The property to deserialize into.
	 * @param ValuePtr Pointer to the property's value in memory.
	 * @param SaveProp Saved property data containing the binary payload.
	 */
	static void DeserializeProperty(FProperty* Property, void* ValuePtr, const FPropertySaveData& SaveProp);

	/**
	 * @brief Finds a property on the object by name and deserializes its saved value.
	 *        Performs type-mismatch validation before writing.
	 * @param Object The UObject to write the property to.
	 * @param SaveProp The saved property data to apply.
	 */
	static void AssignProperty(UObject* Object, const FPropertySaveData& SaveProp);

public:
	/**
	 * @brief Serializes an ISavableActor and all its ISavableObject components into an FObjectSaveData.
	 *
	 * Iterates over all UPROPERTY(SaveGame) on the actor and its savable components,
	 * captures version information, spawn data, and transform.
	 *
	 * @param SavableActor The actor to serialize (must be valid).
	 * @return The serialized data, or NullOpt if the actor is invalid.
	 */
	static TOptional<FObjectSaveData> SerializeActor(ISavableActor* SavableActor);

	/**
	 * @brief Reconstructs an actor from saved data, applying version migration if needed.
	 *
	 * For dynamic actors, spawns a new instance from the saved class.
	 * For static actors, finds the existing actor in StaticSpawnedActorMap.
	 * Runs the migration delegate chain when the saved version differs from the current one,
	 * then restores all properties and component data.
	 *
	 * @param WorldContext Any UObject that can provide a world context (used to get the UWorld).
	 * @param ObjectSaveData The saved actor data to deserialize.
	 * @param Version The current global save version string.
	 * @param StaticSpawnedActorMap Map of UniqueID -> AActor* for level-placed actors.
	 */
	static void DeserializeActor(UObject* WorldContext, const FObjectSaveData& ObjectSaveData, FString Version, const TMap<FString, AActor*>& StaticSpawnedActorMap);
};


/**
 * USaveSubsystem
 *
 * GameInstanceSubsystem that serves as the main entry point for the Save System.
 * Provides high-level SaveWorld / LoadWorld operations that handle the full pipeline:
 * actor discovery, serialization, slot management, deserialization, and version migration.
 *
 * On initialization, iterates over all loaded classes implementing ISavableActor or
 * ISavableObject and calls SetupSaveMigrateLogic on their CDOs to populate the
 * per-class migration delegate maps.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/** Semantic version tuple (Major, Minor, Patch) used for version parsing. */
	using VersionType = TTuple<int, int, int>;

	/** The currently loaded save game object. Transient — not serialized with the subsystem. */
	UPROPERTY(Transient)
	TObjectPtr<ULCUSaveGame> CurrentSave;
	
	/**
	 * @brief Parses a "Major.Minor.Patch" version string into a numeric tuple.
	 * @param Version The version string to parse.
	 * @return The parsed tuple, or NullOpt if the format is invalid.
	 */
	TOptional<VersionType> ExtractVersion(const FString& Version);

	/**
	 * @brief Builds a map of UniqueID -> AActor* for all non-dynamic savable actors in the world.
	 *        Used during deserialization to find level-placed actors by their saved ID.
	 */
	TMap<FString, AActor*> BuildStaticActorSpawnedMap() const;

	/**
	 * @brief Called when the GameInstance initializes.
	 *        Iterates all ISavableActor / ISavableObject classes and calls
	 *        SetupSaveMigrateLogic on their CDOs to register migration delegates.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	
public:

	/**
	 * @brief Saves all savable actors in the current world to the specified slot.
	 *
	 * Discovers all actors implementing ISavableActor, serializes them and their
	 * components, then writes the result to disk via UGameplayStatics::SaveGameToSlot.
	 *
	 * @param SlotName Name of the save slot.
	 * @param Version Global version string to stamp the save with (format: "Major.Minor.Patch").
	 */
	UFUNCTION(BlueprintCallable)
	void SaveWorld(const FName& SlotName, const FString& Version);
	
	/**
	 * @brief Loads a save from the specified slot and restores all actors in the world.
	 *
	 * Reads the save file, then for each saved actor entry calls DeserializeActor
	 * which handles spawning / finding actors, migration, and property restoration.
	 *
	 * @param SlotName Name of the save slot to load.
	 * @param Version Current global version string used for migration comparison.
	 */
	UFUNCTION(BlueprintCallable)
	void LoadWorld(const FName& SlotName, const FString& Version);	
};
