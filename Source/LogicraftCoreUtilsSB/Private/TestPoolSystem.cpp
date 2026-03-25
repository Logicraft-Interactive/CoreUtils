// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "TestPoolSystem.h"

#include "PoolSystem/PoolObject.h"
#include "PoolSystem/PoolSubsystem.h"


void APoolableClassTest::OnReturn_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("Actor Returned"));
}

void APoolableClassTest::OnSpawn_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("Actor Spawned"));	
}

// Sets default values
ATestPoolSystem::ATestPoolSystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATestPoolSystem::BeginPlay()
{
	Super::BeginPlay();

 	PoolObject = GetWorld()->GetSubsystem<UPoolSubsystem>()->CreatePool(FPoolSettings{
		.SpawnClass = ActorToSpawn,
		.WorldContext = GetWorld(),
 		.bAllowResize = false,
		.MinPoolSize = 50
	}, this);

	
	SpawnHolder.Schedule(this, &ThisClass::Spawn, FTimerParameters{.bIsLooping = true, .Rate = 1.f/4});

	ReturnHolder.Schedule(this, &ThisClass::Return, FTimerParameters{.bIsLooping = true, .Rate = 3.f/4});
}

// Called every frame
void ATestPoolSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATestPoolSystem::Spawn()
{
	FTransform Transform;
	Transform.SetLocation({FMath::RandRange(-500.0,500.0),
		FMath::RandRange(-500.0,500.0),
		0});

	auto Test = PoolObject->SpawnFromPool<APoolableClassTest>(Transform);
	PoolableArray.Add(Test);
}

void ATestPoolSystem::Return()
{
	PoolableArray.RemoveAll([](TWeakObjectPtr<APoolableClassTest> Actor)
		{
			return !Actor.IsValid();
		});

	if (PoolableArray.IsEmpty())
		return;
		
	PoolObject->ReturnToPool(PoolableArray[FMath::RandRange(0, PoolableArray.Num() - 1)].Get());
}

