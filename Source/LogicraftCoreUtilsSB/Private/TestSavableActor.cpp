// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestSavableActor.h"

#include "LogCategory.h"
#include "TestSavableActorComponent.h"


FString UTestSavableComponent_Save::GetSaveVersion_Implementation()
{
	return "1.2.0";
}

void UTestSavableComponent_Save::SetupSaveMigrateLogic_Implementation()
{
}

ATestSavableActor::ATestSavableActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SaveComponent = CreateDefaultSubobject<UTestSavableComponent_Save>("SaveComponent");
	SavableActorComponent = CreateDefaultSubobject<UTestSavableActorComponent>("Test Component");
}

void ATestSavableActor::BeginPlay()
{
	Super::BeginPlay();
	if (bDynamicSpawned)
	{
		SaveComponent->SetIsDynamicSpawned(GetClass(), FGuid::NewGuid());
	}
}

void ATestSavableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}