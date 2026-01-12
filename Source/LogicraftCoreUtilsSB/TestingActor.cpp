// Fill out your copyright notice in the Description page of Project Settings.


#include "TestingActor.h"

#include "TimerHolderSubsystem.h"


// Sets default values
ATestingActor::ATestingActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATestingActor::BeginPlay()
{
	Super::BeginPlay();

	UTimerHolderSubsystem::StartTimer(this, []()
	{
		UE_LOG(LogTemp, Warning, TEXT("Start timer log"))
	}, 10.f, false);
}

// Called every frame
void ATestingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

