// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestEventBus.h"

#include "EventBusSubsystem.h"


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

	auto a = [](){};
	TypeTraits::bIsInvocableNoArgs<a>
	
	UEventBusSubsystem::Get(this)->AddUObject(FGameplayTag::EmptyTag, this, &ATestEventBus::Event);
	UEventBusSubsystem::Get(this)->AddUObject(FGameplayTag::EmptyTag, this, &ATestEventBus::EventVoid);
	UEventBusSubsystem::Get(this)->AddLambda(FGameplayTag::EmptyTag, []()
	{
		
	});
}

// Called every frame
void ATestEventBus::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

