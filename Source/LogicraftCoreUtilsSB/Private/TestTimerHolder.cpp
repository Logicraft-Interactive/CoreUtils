// Fill out your copyright notice in the Description page of Project Settings.

#include "LogicraftCoreUtilsSB/Public/TestTimerHolder.h"

#include "TimerHolderSubsystem.h"

ATestTimerHolder::ATestTimerHolder()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestTimerHolder::BeginPlay()
{
	Super::BeginPlay();

	// TimerHandle = UTimerHolderSubsystem::ScheduleTimer(this, []()
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Start timer log"))
	// }, { .Rate = 10.f });
	//
	// UTimerHolderSubsystem::ScheduleTimer(this, []()
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Start timer log"))
	// }, { .Rate = 10.f }, ETimerScope::ContextBound);
	//
	// UTimerHolderSubsystem::ScheduleTimer(this, &ATestTimerHolder::TimerFunction, { .Rate = 10.f });
}

void ATestTimerHolder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// UTimerHolderSubsystem::CancelTimer(this, TimerHandle);
	// UTimerHolderSubsystem::CancelTimer(this);
	// UTimerHolderSubsystem::CancelTimer(this, ETimerScope::ContextBound);
}

void ATestTimerHolder::TimerFunction()
{
	UE_LOG(LogTemp, Warning, TEXT("Start timer log in a function"))
}

