// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PoolSettings.generated.h"

class UPoolable;
/**
 * 
 */
USTRUCT(Blueprintable, BlueprintType)
struct LOGICRAFTCOREUTILS_API FPoolSettings 
{
	GENERATED_BODY()
	 
	TSubclassOf<AActor> SpawnClass;
	 
	TWeakObjectPtr<UWorld> WorldContext = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	bool bAllowResize = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAutoShrink = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	int MinPoolSize = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	float AutoShrinkUpdateTime = 15.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	float ShrinkObjectAfterReturnTime = 30.f;
};
 
UCLASS(Blueprintable, BlueprintType)
class UPoolSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public: 	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	bool bAllowResize = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAutoShrink = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	int MinPoolSize = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float AutoShrinkUpdateTime = 15.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)	
	float ShrinkObjectAfterReturnTime = 30.f;
	
};
