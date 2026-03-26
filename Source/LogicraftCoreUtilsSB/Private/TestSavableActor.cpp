// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestSavableActor.h"

#include "Chain.h"
#include "LogCategory.h"
#include "TestSavableActorComponent.h"
#include "SaveSystem/SaveSubsystem.h"


FString UTestSavableComponent_Save::GetSaveVersion_Implementation()
{
	return "1.2.0";
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
	
	Chain::StartChain(GetGameInstance())
	.Transform([](UGameInstance* GameInstance)
	{
		return GameInstance->GetSubsystem<USaveSubsystem>();
	})
	.Execute([](USaveSubsystem* SaveSubsystem)
	{
		SaveSubsystem->SaveWorld(TEXT("SlotTest"), TEXT("1.0.0"));
	});
}