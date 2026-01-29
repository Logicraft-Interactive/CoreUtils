#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

// Project Includes
#include "PoolSystem/PoolObject.h"
#include "PoolSystem/PoolSettings.h"
#include "PoolSystem/PoolSubsystem.h"
#include "Test/PoolableTestActor.h"

// --- TEST CONSTANTS ---
static const int32 BenchmarkCount = 100000;
static const int32 MinPoolSizeTest = 10;

// --- TEST GROUP DEFINITION ---
// Flags: Editor Context (needs World), Product Filter (standard tests)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoolSystemReuseTest, "Project.PoolSystem.Reuse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoolSystemPerformanceTest, "Project.PoolSystem.Performance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoolSystemCapacityTest, "Project.PoolSystem.Capacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)


/**
 * Test 1: Reuse Integrity
 * Verifies that returning an actor to the pool and spawning again returns the exact same memory address.
 */
bool FPoolSystemReuseTest::RunTest(const FString& Parameters)
{
    // 1. Get World Context
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return false; // Cannot test without a world
    }

    // 2. Get Subsystem
    UPoolSubsystem* PoolSubsystem = World->GetSubsystem<UPoolSubsystem>();
    TestNotNull("Pool Subsystem should exist", PoolSubsystem);

    // 3. Configure Pool
    FPoolSettings Settings;
    Settings.SpawnClass = APoolableTestActor::StaticClass();
    Settings.MinPoolSize = 5;
    Settings.bAllowResize = true;
    Settings.WorldContext = World;

    UPoolObject* Pool = PoolSubsystem->CreatePool(Settings);
    TestNotNull("Pool Object created", Pool);

    TArray<AActor*> SpawnedActor;
    for (int i = 0; i < Settings.MinPoolSize; ++i)
    {        
        AActor* ActorA = Pool->SpawnFromPool(FTransform::Identity);
        TestNotNull("Actor A should be valid", ActorA);
        SpawnedActor.Add(ActorA);
    }
    
    // 4. Spawn first actor
    
    // 5. Return actor to pool
    for (int i = 0; i < Settings.MinPoolSize; ++i)
    {        
        Pool->ReturnToPool(SpawnedActor[i]);
    }


    for (int i = 0; i < Settings.MinPoolSize; ++i)
    {
        AActor* ActorB = Pool->SpawnFromPool(FTransform::Identity);
        TestNotNull("Actor B should be valid", ActorB);

        // 7. ASSERT: Actor A and Actor B should be the same pointer
        TestEqual("The pool should reuse the same actor instance", SpawnedActor[i], ActorB);
    }
    // 6. Spawn again


    // Cleanup (Optional, depending on if you want to keep the actors in the editor world)
    // In a pure unit test environment, we might tear down the world or actors here.
    
    return true;
}

/**
 * Test 2: Performance Benchmark
 * Compares standard SpawnActor vs SpawnFromPool.
 */
bool FPoolSystemPerformanceTest::RunTest(const FString& Parameters)
{
    auto GetTime = [](auto Func)
    {
        double StartTimeNative = FPlatformTime::Seconds();
        Func();
        double EndTimeNative = FPlatformTime::Seconds();
        return EndTimeNative - StartTimeNative;
    };

    
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return false;

    UPoolSubsystem* PoolSubsystem = World->GetSubsystem<UPoolSubsystem>();
    
    // Setup Pool
    FPoolSettings Settings;
    Settings.SpawnClass = APoolableTestActor::StaticClass();
    Settings.MinPoolSize = BenchmarkCount; // Pre-warm the pool to test pure retrieval speed
    Settings.bAllowResize = true;
    Settings.WorldContext = World;

    UPoolObject* Pool = nullptr;

    double InitTime = GetTime([&]
        {
            Pool = PoolSubsystem->CreatePool(Settings);
        });

    // --- BENCHMARK: Native SpawnActor --- 
    TArray<AActor*> NativeActors;
    NativeActors.Reserve(BenchmarkCount);

   
    double DurationNative = GetTime([&]
    {
        for (int32 i = 0; i < BenchmarkCount; ++i)
        {
            AActor* NewActor = World->SpawnActor<APoolableTestActor>(FVector::ZeroVector, FRotator::ZeroRotator);
            NativeActors.Add(NewActor);
        } 
    });

    // Cleanup Native Actors
    for (AActor* Act : NativeActors)
    {
        if (Act) Act->Destroy();
    }

    // --- BENCHMARK: Pool Spawn ---
    TArray<AActor*> PoolActors;
    PoolActors.Reserve(BenchmarkCount);
    
    double DurationPool = GetTime([&]
    {
        for (int32 i = 0; i < BenchmarkCount; ++i)
        {
            AActor* PooledActor = Pool->SpawnFromPool(FTransform::Identity);
            PoolActors.Add(PooledActor);
        }
    });

    for (AActor* PoolActor : PoolActors)
    {
        Pool->ReturnToPool(PoolActor);
    }
    
    // Log Results
    UE_LOG(LogTemp, Display, TEXT("Performance Test Results (%d iterations):"), BenchmarkCount);
    UE_LOG(LogTemp, Display, TEXT(" - Native Spawn: %f seconds"), DurationNative);
    UE_LOG(LogTemp, Display, TEXT(" - Pool Spawn:   %f seconds"), DurationPool);

    // Assert that pooling is faster (Note: This might fail on very small counts or if the pool needs to resize dynamically during the loop)
    // We use a warning instead of an error because in some editor contexts, GC overhead can skew results.
    if (DurationPool > DurationNative)
    {
        UE_LOG(LogTemp, Warning, TEXT("Pool was slower than native spawn. Check if MinPoolSize was sufficient to avoid allocation during runtime."));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("SUCCESS: Pool is %.2fx faster than native spawn with a init time of %f seconds."), DurationNative / DurationPool, InitTime);
    }

    return true;
}

/**
 * Test 3: Capacity and Resizing
 * Checks if the pool respects MinPoolSize and can expand beyond it.
 */
bool FPoolSystemCapacityTest::RunTest(const FString& Parameters)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return false;

    UPoolSubsystem* PoolSubsystem = World->GetSubsystem<UPoolSubsystem>();

    // Setup Pool with small size
    int32 SmallSize = 3;
    FPoolSettings Settings;
    Settings.SpawnClass = APoolableTestActor::StaticClass();
    Settings.MinPoolSize = SmallSize;
    Settings.bAllowResize = true; // Allow expansion
    Settings.WorldContext = World;

    UPoolObject* Pool = PoolSubsystem->CreatePool(Settings);

    TArray<AActor*> ActiveActors;

    // 1. Fill the pool
    for(int32 i = 0; i < SmallSize; i++)
    {
        AActor* Act = Pool->SpawnFromPool(FTransform::Identity);
        TestNotNull("Actor should spawn within limit", Act);
        ActiveActors.Add(Act);
    }

    // 2. Try to spawn ONE MORE (Trigger Resize)
    AActor* ExtraActor = Pool->SpawnFromPool(FTransform::Identity);
    
    if (Settings.bAllowResize)
    {
        TestNotNull("Pool should expand and return valid actor", ExtraActor);
        ActiveActors.Add(ExtraActor);
    }
    else
    {
        TestNull("Pool should NOT spawn if resize is false", ExtraActor);
    }

    // 3. Return everything
    for (AActor* Act : ActiveActors)
    {
        Pool->ReturnToPool(Act);
    }

    // 4. Verify CanSpawn
    TestTrue("Pool should be ready to spawn again", Pool->CanSpawn());

    return true;
}
#endif