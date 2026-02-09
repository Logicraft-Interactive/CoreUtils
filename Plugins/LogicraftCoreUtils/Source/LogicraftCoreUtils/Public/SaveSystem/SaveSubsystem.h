// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	using VersionType = TTuple<int, int, int>;
	TOptional<VersionType> ExtractVersion(const FString& Version);
	
public:

	void SaveWorld(const FString& Version);	
};
