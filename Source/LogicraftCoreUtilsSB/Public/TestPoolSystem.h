// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerHolder.h"
#include "GameFramework/Actor.h"
#include "PoolSystem/Poolable.h"
#include "TestPoolSystem.generated.h"

class UPoolObject;

UCLASS()
class
APoolableClassTest : public AActor, public IPoolable
{
public:
	virtual void OnReturn_Implementation() override;
	virtual void OnSpawn_Implementation() override;

private:
	GENERATED_BODY()

public:
	
};


UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestPoolSystem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestPoolSystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, meta = (MustImplement = "/Script/LogicraftCoreUtils.Poolable"))
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(Transient)
	UPoolObject* PoolObject;

	FTimerHolder SpawnHolder;
	FTimerHolder ReturnHolder;

	TArray<TWeakObjectPtr<APoolableClassTest>> PoolableArray;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Spawn();
	void Return();
};
