// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "Components/ActorComponent.h"
#include "SaveableComponent.generated.h"

class USaveableComponent;

/**
 * @brief Dynamic delegate signature for component version migration callbacks.
 *
 * Called during deserialization when a component's saved version does not match
 * the current version. Receives the component being migrated, the source and target
 * version strings, and the old property array for remapping.
 */
DECLARE_DYNAMIC_DELEGATE_FourParams(FSaveableComponentMigrateEventSignature, USaveableComponent*, Component, FString, FromVersion, FString, ToVersion, const TArray<FPropertySaveData>&, OldPropertyArray);

/**
 * USaveableComponent
 *
 * Base class for actor components whose UPROPERTY(SaveGame) properties should
 * be serialized alongside their owning actor. Provides independent versioning
 * and migration support per component class.
 *
 * To use: create a C++ or Blueprint subclass, add UPROPERTY(SaveGame) variables,
 * and optionally override GetSaveVersion / SetupSaveMigrateLogic / lifecycle events.
 *
 * The owning actor must also have a USaveComponent attached for the save system
 * to discover and serialize this component.
 *
 * Version migration uses a per-class static delegate map, identical to USaveComponent.
 */
UCLASS(ClassGroup=(SaveSystem), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName = "Saveable Component")
class SAVESYSTEM_API USaveableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	using FDelegateMapType = TMap<FString, TFunction<FString(USaveableComponent*, FString, const TArray<FPropertySaveData>&)>>;

	USaveableComponent();

	/**
	 * @brief Returns the current save version string for this component.
	 * Override in subclass (C++ or BP) to provide the version.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Save System", DisplayName = "Get Save Version")
	FString GetSaveVersion();

	/**
	 * @brief Entry point for registering version migration delegates.
	 * Called once on each class CDO during USaveSubsystem::Initialize.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Save System", DisplayName = "Setup Save Migrate Logic")
	void SetupSaveMigrateLogic();

	/**
	 * @brief Registers a Blueprint migration delegate for a specific version transition.
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	 * @param Delegate Dynamic delegate to execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void AddMigrateDelegate(const FString& FromVersion, const FString& ToVersion, FSaveableComponentMigrateEventSignature Delegate);

	/**
	 * @brief Registers a C++ lambda as a migration delegate for a specific version transition.
	 *
	 * @tparam Func Callable accepting (USaveableComponent*, FString FromVersion, FString ToVersion, const TArray<FPropertySaveData>&).
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	 * @param Lambda The migration logic to execute.
	 */
	template<typename Func>
	void AddMigrateDelegateLambda(const FString& FromVersion, const FString& ToVersion, Func&& Lambda)
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
		
		MigratesDelegateMap[Class].Add(FromVersion, [MigrateLogic = Forward<Func>(Lambda), ToVersion](USaveableComponent* Comp, FString From, const TArray<FPropertySaveData>& PropertyArray)
		{
			MigrateLogic(Comp, From, ToVersion, PropertyArray);
			return ToVersion;
		});
	}

	/** @brief Returns the migration delegate map for the given component's class. */
	static const FDelegateMapType& GetMigrateDelegateMap(const UObject* Object);

	// ---- Serialization lifecycle callbacks ----

	UFUNCTION(BlueprintNativeEvent, Category = "Save System")
	void OnPreSave();

	UFUNCTION(BlueprintNativeEvent, Category = "Save System")
	void OnPostSave();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Save System")
	void OnPreLoad();

	UFUNCTION(BlueprintNativeEvent, Category = "Save System")
	void OnPostLoad();

protected:
	inline static TMap<UClass*, FDelegateMapType> MigratesDelegateMap{};
	inline static FDelegateMapType EmptyMap{};
};
