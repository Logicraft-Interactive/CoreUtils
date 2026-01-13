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

	UTimerHolderSubsystem::ScheduleTimer(this, []()
	{
		UE_LOG(LogTemp, Warning, TEXT("Start timer log"))
	}, { .Rate = 10.f });

	UTimerHolderSubsystem::ScheduleTimer(this, &ATestingActor::TimerFunction, { .Rate = 10.f });
}

void ATestingActor::TimerFunction()
{
	UE_LOG(LogTemp, Warning, TEXT("Start timer log in a function"))
}

// Called every frame
void ATestingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UTimerHolderSubsystem::CancelTimer(this, ETimerScope::ContextBound);
	UTimerHolderSubsystem::CancelTimer(this);
}

