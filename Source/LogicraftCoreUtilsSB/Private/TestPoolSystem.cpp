// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestPoolSystem.h"

#include "PoolSystem/PoolObject.h"
#include "PoolSystem/PoolSubsystem.h"


void APoolableClassTest::OnReturn_Implementation()
{
	UE_LOG(LogTemp, Error, TEXT("Actor Spawned"));
}

void APoolableClassTest::OnSpawn_Implementation()
{
	UE_LOG(LogTemp, Error, TEXT("Actor Returned"));	
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
		.MinPoolSize = 50
	});

	
	TimerHolder.Schedule([&]
	{
		FTransform Transform;
		Transform.SetLocation({FMath::RandRange(-500.0,500.0),
			FMath::RandRange(-500.0,500.0),
			0});

			
		PoolObject->Spawn<ATestPoolSystem>(Transform);
	}, FTimerParameters{.bIsLooping = true, .Rate = 1.f});
}

// Called every frame
void ATestPoolSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

