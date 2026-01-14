// Fill out your copyright notice in the Description page of Project Settings.


#include "TestLinq.h"
#include "Linq.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#define LOG(Text, ...) UE_LOG(LogTemp, Error, TEXT(Text), __VA_ARGS__)
// Sets default values
ATestLinq::ATestLinq()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}
 
// Called when the game starts or when spawned
void ATestLinq::BeginPlay()
{
	Super::BeginPlay(); 
 
	TArray<int> Numbers;
	for (int i = 0; i < 20; ++i)
	{
		Numbers.Add(FMath::RandRange(1,50));
	}
 
	auto Arr = Linq::Start(Numbers)
	.Select([](int a){return static_cast<double>(a);})
	.Apply([](double a){return a * 5;})
	.Execute([](double a){LOG("%d", (int)a);})
	.ToArray();

 

	if (auto Min = Linq::Start(Arr).Min())
	{
		LOG("Min : %d", (int)(Min ? *Min : -999999));
	}
	//else LOG("Collection is empty");
	
	auto BeforeComps = GetComponents().Array();
	auto Comps = Linq::Start(GetComponents().Array())
	.IsValid()
	.Cast<UBoxComponent>()
	.Execute([](UBoxComponent* BoxComponent){BoxComponent->SetBoxExtent({500,500,500});})
	.ToArray();

	auto FirstComp = Linq::Start(Comps).First([](UBoxComponent* BoxComponent){return IsValid(BoxComponent);});
	LOG("Expected Box : %s", *FirstComp->GetName());
	auto LastComp = Linq::Start(Comps).Last([](UBoxComponent* BoxComponent){return IsValid(BoxComponent);});
	LOG("Expected Box3 : %s", *LastComp->GetName());
	
	auto SelectorExtentXResult = Linq::Start(Comps)
	.Sum([](UBoxComponent* BoxComponent){return BoxComponent->GetScaledBoxExtent().X;});
	
	auto ExtentXResult = Linq::Start(Comps)
	.Select([](UBoxComponent* BoxComponent){return BoxComponent->GetScaledBoxExtent().X;})
	.Sum();
	
	LOG("Expected 2000 : %d", (int)ExtentXResult);
	LOG("Expected 2000 : %d", (int)SelectorExtentXResult);

	auto AnyBox = Linq::Start(Comps)
	.Any([](UBoxComponent* BoxComponent){return  FString(BoxComponent->GetName()).Contains(UTF8TEXT("1"));});

	LOG("Expected 1 : %d", (int)AnyBox);

	auto AllBox = Linq::Start(Comps)
	.All([](UBoxComponent* BoxComponent)
	{ 
		return FString(BoxComponent->GetName()).Contains(TEXT("Box"));
	});

	LOG("Expected 1 : %d", (int)AllBox);

	auto NotAllBox = Linq::Start(Comps)
	.All([](UBoxComponent* BoxComponent)
	{ 
		return FString(BoxComponent->GetName()).Contains(TEXT("Box1"));
	});

	LOG("Expected 0 : %d", (int)NotAllBox);

	TArray BaseTakeAndSkipArray {1,2,3,4,5,6,7,8,9,10};

	auto TakeArray = Linq::Start(BaseTakeAndSkipArray)
	.Take(5)
	.ToArray();

	auto SkipArray = Linq::Start(BaseTakeAndSkipArray)
	.Skip(5)
	.ToArray();

	auto PrintArray = [](TArray<int> Array, auto Message)
	{
		LOG("%s", Message);
		for (const auto& It : Array)
		{
			LOG("%d", It);
		}
	};

	PrintArray(TakeArray, TEXT("Printing Take Array"));
	PrintArray(SkipArray, TEXT("Printing Skip Array"));
	
}

// Called every frame
void ATestLinq::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

