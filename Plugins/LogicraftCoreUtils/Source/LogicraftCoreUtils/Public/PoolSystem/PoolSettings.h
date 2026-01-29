// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataAsset.h"
#include "PoolSettings.generated.h"

class UPoolable;

/**
 * 
 */
USTRUCT(Blueprintable, BlueprintType)
struct LOGICRAFTCOREUTILS_API FPoolSettings 
{
	GENERATED_BODY()
	 
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite, meta = (MustImplement = "Poolable"))	
	TSubclassOf<AActor> SpawnClass;
	 
	TWeakObjectPtr<UWorld> WorldContext{nullptr};
	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	bool bAllowResize{true};
	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)
	bool bAutoShrink{true};
	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	int MinPoolSize{30};
	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	float AutoShrinkUpdateTime{15.f};
	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	float ShrinkObjectAfterReturnTime{30.f};
};
 
UCLASS(Blueprintable, BlueprintType)
class LOGICRAFTCOREUTILS_API UPoolSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public: 	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	FPoolSettings PoolSettings;
	
};
