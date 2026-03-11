// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestEventBus.h"
#include "EventBus.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test_Actor_Bus, "Test.Event.Bus", "Tag for event bus testing.");

// Sets default values
ATestEventBus::ATestEventBus()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATestEventBus::BeginPlay()
{
	Super::BeginPlay();

	EventBus = UEventBus::Get(this);
	Handle = EventBus->AddLambda(Test_Actor_Bus, [](float a){});
	EventBus->Broadcast(Test_Actor_Bus, 10, 10.f);
}

void ATestEventBus::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	EventBus->Remove(Test_Actor_Bus, Handle);
}
