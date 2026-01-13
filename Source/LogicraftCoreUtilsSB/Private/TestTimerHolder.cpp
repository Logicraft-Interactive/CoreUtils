// Fill out your copyright notice in the Description page of Project Settings.

#include "LogicraftCoreUtilsSB/Public/TestTimerHolder.h"

ATestTimerHolder::ATestTimerHolder()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestTimerHolder::BeginPlay()
{
	Super::BeginPlay();

	LambdaTimerHolder.Schedule([]
	{
		UE_LOG(LogTemp, Warning, TEXT("Start timer log"))
	}, { .Rate = 10.f });

	MemberTimerHolder.Schedule(this, &ThisClass::TimerFunction, { .Rate = 10.f });
}

void ATestTimerHolder::TimerFunction()
{
	UE_LOG(LogTemp, Warning, TEXT("Start timer log in a function"))
}

