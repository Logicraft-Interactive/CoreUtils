// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestEventBus.generated.h"

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestEventBus : public AActor
{
	GENERATED_BODY()

	FDelegateHandle Handle;
	
public:
	// Sets default values for this actor's properties
	ATestEventBus();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
