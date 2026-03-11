#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "SaveSystem/SaveSubsystem.h"
#include "SaveSystem/SaveData.h"
#include "SaveSystem/SavableActor.h"
#include "Test/SavableTestActor.h"

// --- TEST CONSTANTS ---
static const int32 SaveBenchmarkCount = 5000;
static const FString BenchmarkVersion = TEXT("1.0.0");
static const FName BenchmarkSlotName = TEXT("__SaveSystemBenchmark__");

// --- TEST DEFINITION ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSystemPerformanceTest, "Logicraft.SaveSystem.Performance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

bool FSaveSystemPerformanceTest::RunTest(const FString& Parameters)
{
	auto GetTime = [](auto Func)
	{
		const double Start = FPlatformTime::Seconds();
		Func();
		const double End = FPlatformTime::Seconds();
		return End - Start;
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available"));
		return false;
	}

	// --- SPAWN ACTORS (marked as dynamic, since they are runtime-spawned) ---
	TArray<AActor*> SpawnedActors;
	SpawnedActors.Reserve(SaveBenchmarkCount);

	const double DurationSpawn = GetTime([&]
	{
		for (int32 i = 0; i < SaveBenchmarkCount; ++i)
		{
			AActor* Actor = World->SpawnActor<ASavableTestActor>(FVector(i * 100.f, 0.f, 0.f), FRotator::ZeroRotator);
			if (Actor)
			{
				Cast<ISavableActor>(Actor)->SetIsDynamicSpawned(Actor->GetClass());
				SpawnedActors.Add(Actor);
			}
		}
	});

	TestEqual(TEXT("All actors should have spawned"), SpawnedActors.Num(), SaveBenchmarkCount);

	// --- BENCHMARK 1: Serialization ---
	TArray<FObjectSaveData> SerializedData;
	SerializedData.Reserve(SaveBenchmarkCount);

	const double DurationSerialize = GetTime([&]
	{
		for (AActor* Actor : SpawnedActors)
		{
			ISavableActor* Savable = Cast<ISavableActor>(Actor);
			if (auto Data = FSaveSerializer::SerializeActor(Savable))
			{
				SerializedData.Add(MoveTemp(*Data));
			}
		}
	});

	TestEqual(TEXT("All actors should have been serialized"), SerializedData.Num(), SaveBenchmarkCount);

	// --- BENCHMARK 2: Full save to slot (serialization + disk write) ---
	const double DurationSaveToSlot = GetTime([&]
	{
		ULCUSaveGame* SaveGame = Cast<ULCUSaveGame>(UGameplayStatics::CreateSaveGameObject(ULCUSaveGame::StaticClass()));
		SaveGame->ObjectsData = MoveTemp(SerializedData);
		SaveGame->SaveSlotName = BenchmarkSlotName;
		SaveGame->SaveTimeStamp = FDateTime::Now();
		SaveGame->GlobalSaveVersion = BenchmarkVersion;

		UGameplayStatics::SaveGameToSlot(SaveGame, BenchmarkSlotName.ToString(), 0);
	});

	// --- BENCHMARK 3: Load from slot ---
	ULCUSaveGame* LoadedSave = nullptr;

	const double DurationLoadFromSlot = GetTime([&]
	{
		LoadedSave = Cast<ULCUSaveGame>(UGameplayStatics::LoadGameFromSlot(BenchmarkSlotName.ToString(), 0));
	});

	TestNotNull(TEXT("Save game should load from slot"), LoadedSave);

	// --- BENCHMARK 4: Deserialization (dynamic actors are re-spawned) ---
	TMap<FString, AActor*> EmptyStaticMap;

	const double DurationDeserialize = GetTime([&]
	{
		if (!LoadedSave) return;
		for (const FObjectSaveData& ObjectData : LoadedSave->ObjectsData)
		{
			FSaveSerializer::DeserializeActor(SpawnedActors[0], ObjectData, BenchmarkVersion, EmptyStaticMap);
		}
	});

	// --- LOG RESULTS ---
	const double SerializePerActor = (DurationSerialize / SaveBenchmarkCount) * 1000.0;
	const double DeserializePerActor = (DurationDeserialize / SaveBenchmarkCount) * 1000.0;
	const double TotalSaveTime = DurationSerialize + DurationSaveToSlot;
	const double TotalLoadTime = DurationLoadFromSlot + DurationDeserialize;

	UE_LOG(LogTemp, Display, TEXT("=== Save System Benchmark (%d actors) ==="), SaveBenchmarkCount);
	UE_LOG(LogTemp, Display, TEXT(" - Spawn:              %f s"), DurationSpawn);
	UE_LOG(LogTemp, Display, TEXT(" - Serialization:      %f s  (%.4f ms/actor)"), DurationSerialize, SerializePerActor);
	UE_LOG(LogTemp, Display, TEXT(" - Save to slot:       %f s"), DurationSaveToSlot);
	UE_LOG(LogTemp, Display, TEXT(" - Total save:         %f s"), TotalSaveTime);
	UE_LOG(LogTemp, Display, TEXT(" - Load from slot:     %f s"), DurationLoadFromSlot);
	UE_LOG(LogTemp, Display, TEXT(" - Deserialization:    %f s  (%.4f ms/actor)"), DurationDeserialize, DeserializePerActor);
	UE_LOG(LogTemp, Display, TEXT(" - Total load:         %f s"), TotalLoadTime);

	// --- CLEANUP (original + deserialized actors) ---
	TArray<AActor*> AllTestActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASavableTestActor::StaticClass(), AllTestActors);
	for (AActor* Actor : AllTestActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}

	UGameplayStatics::DeleteGameInSlot(BenchmarkSlotName.ToString(), 0);

	return true;
}

// =============================================================================
// Test: Single version migration
// Migration delegates are per-class (static map keyed by UClass*), so they
// apply to any instance including dynamically spawned actors.
// =============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSystemVersionMigrationTest, "Project.SaveSystem.VersionMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveSystemVersionMigrationTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available"));
		return false;
	}

	AActor* RawActor = World->SpawnActor<ASavableTestActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Actor should spawn"), RawActor);

	ISavableActor* Savable = Cast<ISavableActor>(RawActor);
	TestNotNull(TEXT("Actor should implement ISavableActor"), Savable);
	Savable->SetIsDynamicSpawned(RawActor->GetClass());

	TOptional<FObjectSaveData> OptData = FSaveSerializer::SerializeActor(Savable);
	TestTrue(TEXT("Serialization should succeed"), OptData.IsSet());

	FObjectSaveData SaveData = MoveTemp(*OptData);
	SaveData.SaveVersion = TEXT("0.9.0");

	bool bMigrationCalled = false;
	Savable->AddMigrateDelegateLambda(TEXT("0.9.0"), TEXT("1.0.0"),
		[&bMigrationCalled](AActor* Actor, FString From, FString To, const TArray<FPropertySaveData>& OldProps)
		{
			bMigrationCalled = true;
		});

	TMap<FString, AActor*> EmptyStaticMap;
	FSaveSerializer::DeserializeActor(RawActor, SaveData, TEXT("1.0.0"), EmptyStaticMap);

	TestTrue(TEXT("Migration delegate 0.9.0 -> 1.0.0 should have been called"), bMigrationCalled);

	TArray<AActor*> AllTestActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASavableTestActor::StaticClass(), AllTestActors);
	for (AActor* Actor : AllTestActors)
	{
		if (Actor) Actor->Destroy();
	}

	return true;
}

// =============================================================================
// Test: Chained version migration (0.8.0 -> 0.9.0 -> 1.0.0)
// Verifies that the migration chain executes delegates in order.
// =============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSystemVersionMigrationChainTest, "Project.SaveSystem.VersionMigrationChain", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveSystemVersionMigrationChainTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available"));
		return false;
	}

	AActor* RawActor = World->SpawnActor<ASavableTestActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Actor should spawn"), RawActor);

	ISavableActor* Savable = Cast<ISavableActor>(RawActor);
	TestNotNull(TEXT("Actor should implement ISavableActor"), Savable);
	Savable->SetIsDynamicSpawned(RawActor->GetClass());

	TOptional<FObjectSaveData> OptData = FSaveSerializer::SerializeActor(Savable);
	TestTrue(TEXT("Serialization should succeed"), OptData.IsSet());

	FObjectSaveData SaveData = MoveTemp(*OptData);
	SaveData.SaveVersion = TEXT("0.8.0");

	TArray<FString> MigrationOrder;

	Savable->AddMigrateDelegateLambda(TEXT("0.8.0"), TEXT("0.9.0"),
		[&MigrationOrder](AActor* Self, FString From, FString To, const TArray<FPropertySaveData>& OldProps)
		{
			MigrationOrder.Add(FString::Printf(TEXT("%s->%s"), *From, *To));
		});

	Savable->AddMigrateDelegateLambda(TEXT("0.9.0"), TEXT("1.0.0"),
		[&MigrationOrder](AActor* Self, FString From, FString To, const TArray<FPropertySaveData>& OldProps)
		{
			MigrationOrder.Add(FString::Printf(TEXT("%s->%s"), *From, *To));
		});

	TMap<FString, AActor*> EmptyStaticMap;
	FSaveSerializer::DeserializeActor(RawActor, SaveData, TEXT("1.0.0"), EmptyStaticMap);

	TestEqual(TEXT("Two migration steps should have been executed"), MigrationOrder.Num(), 2);

	if (MigrationOrder.Num() == 2)
	{
		TestEqual(TEXT("First migration should be 0.8.0->0.9.0"), MigrationOrder[0], TEXT("0.8.0->0.9.0"));
		TestEqual(TEXT("Second migration should be 0.9.0->1.0.0"), MigrationOrder[1], TEXT("0.9.0->1.0.0"));
	}

	TArray<AActor*> AllTestActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASavableTestActor::StaticClass(), AllTestActors);
	for (AActor* Actor : AllTestActors)
	{
		if (Actor) Actor->Destroy();
	}

	return true;
}
#endif
