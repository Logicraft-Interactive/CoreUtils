// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestEventBus.generated.h"

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestEventBus : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestEventBus();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	inline void Event(int myInt)
	{
		UE_LOG(LogTemp, Warning, TEXT("Int"))
	}
	inline void EventVoid()
	{
		UE_LOG(LogTemp, Warning, TEXT("Void"))
	}
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
