// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestSavableActor.h"

#include "LogCategory.h"
#include "TestSavableActorComponent.h"


// Sets default values
ATestSavableActor::ATestSavableActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SavableActorComponent = CreateDefaultSubobject<UTestSavableActorComponent>("Test Component");


}

// Called when the game starts or when spawned
void ATestSavableActor::BeginPlay()
{
	Super::BeginPlay();
	if (bDynamicSpawned)
	{
		SetIsDynamicSpawned(GetClass());
	}
}

FString ATestSavableActor::GetVersion_Implementation()
{
		return "1.2.0";
}

void ATestSavableActor::SetupSaveMigrateLogic_Implementation()
{
}

// Called every frame
void ATestSavableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

