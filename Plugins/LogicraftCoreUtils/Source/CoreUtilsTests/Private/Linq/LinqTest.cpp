#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Linq.h"
#include "LinqTestObject.h"
#include "UObject/Package.h"

// Define a simple test category
DEFINE_LOG_CATEGORY_STATIC(LogLinqTest, Log, All);

// 1. Basic Integer Tests: Filtering, Projection, Aggregation
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqIntegerTest, "Logicraft.Linq.IntegerLogic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
 

bool FLinqIntegerTest::RunTest(const FString& Parameters)
{
	// Setup
	TArray<int32> Numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	// Test: Where (Filter even numbers)
	TArray<int32> EvenNumbers = Linq::Start(Numbers)
		.Where([](int32 N) { return N % 2 == 0; })
		.ToArray();

	TestEqual("Where should filter even numbers", EvenNumbers.Num(), 5);
	TestEqual("First even number should be 2", EvenNumbers[0], 2);

	// Test: Select (Square numbers)
	TArray<int32> Squared = Linq::Start(Numbers)
		.Take(3)
		.Select([](int32 N) { return N * N; })
		.ToArray();

	TestEqual("Select should return 3 items due to Take(3)", Squared.Num(), 3);
	TestEqual("First square should be 1", Squared[0], 1);
	TestEqual("Third square should be 9", Squared[2], 9);

	// Test: Sum
	int32 SumOfFirstFive = Linq::Start(Numbers)
		.Take(5)
		.Sum();
	
	TestEqual("Sum of 1 to 5 should be 15", SumOfFirstFive, 15);

	// Test: Count
	int32 Count = Linq::Start(Numbers)
	.Take(4)
	.Count([](int32 N){return N % 2 == 0;});

	TestEqual("Count of even numbers from 1 to 4 should return 2", Count, 2);
	
	// Test: Any / All
	bool bHasNegative = Linq::Start(Numbers).Any([](int32 N) { return N < 0; });
	bool bAllPositive = Linq::Start(Numbers).All([](int32 N) { return N > 0; });

	TestFalse("Collection should not have negative numbers", bHasNegative);
	TestTrue("Collection should have all positive numbers", bAllPositive);

	return true;
}

// 2. Complex Object Tests: Sorting and Edge Cases
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqSortingTest, "Logicraft.Linq.Sorting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinqSortingTest::RunTest(const FString& Parameters)
{
	TArray<int32> Unsorted = { 5, 1, 9, 3 };

	// Test: OrderBy (Ascending)
	TArray<int32> Sorted = Linq::Start(Unsorted)
		.OrderBy()
		.ToArray();

	TestEqual("First item should be 1", Sorted[0], 1);
	TestEqual("Last item should be 9", Sorted.Last(), 9);

	// Test: OrderBy (Descending via Comparator)
	TArray<int32> Descending = Linq::Start(Unsorted)
		.OrderBy([](int32 A, int32 B) { return A > B; }) // Inverse comparator
		.ToArray();

	TestEqual("First item should be 9", Descending[0], 9);
	
	// Test: First / FirstOrDefault
	TOptional<int32> FirstMatch = Linq::Start(Unsorted).First([](int32 N) { return N > 4; });
	TestEqual("First number > 4 should be 5", FirstMatch.IsSet() ? *FirstMatch : -1, 5);

	int32 Missing = Linq::Start(Unsorted).FirstOrDefault([](int32 N) { return N > 100; }, -1);
	TestEqual("Should return default value -1 when not found", Missing, -1);

	return true;
}

// 3. Unreal Engine Specific Tests: UObjects, Cast, IsValid
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqUObjectTest, "Logicraft.Linq.UObjectFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinqUObjectTest::RunTest(const FString& Parameters)
{
	// Setup: Create a transient package to hold objects
	UPackage* Package = GetTransientPackage();
	
	// Create mixed objects
	UObject* ValidObj = NewObject<ULinqTestObject>(Package);
	UObject* NullObj = nullptr;
	
	// Note: We use UObject but pretend some are different for the Cast test logic. 
	// In a real scenario, we would use different UClasses. 
	// Here we test that the mechanism works with basic UObjects.
	
	TArray<UObject*> ObjectList;
	ObjectList.Add(ValidObj);
	ObjectList.Add(NullObj);
	ObjectList.Add(ValidObj);

	// Test: IsValid
	// The library's IsValidIterator checks Unreal's IsValid() function
	TArray<UObject*> ValidOnly = Linq::Start(ObjectList)
		.IsValid()
		.ToArray();

	TestEqual("IsValid should remove nullptr", ValidOnly.Num(), 2);

	// Test: Apply (Side Effects)
	// We will use a counter to verify Apply was called on valid objects
	int32 CallCount = 0;
	Linq::Start(ValidOnly)
		.Execute([&](UObject* Obj) {
			CallCount++;
		})
		.End(); // Force execution

	TestEqual("Execute should have been called twice", CallCount, 2);

	// Test: Cast
	// Since we only have UObject, Cast<UObject> should succeed for valid ones.
	// Cast<UActorComponent> should fail (return null) and be filtered out by TCastIterator logic? 
	// Let's verify TCastIterator implementation: it does "if (CurrentValue = Cast...)"
	// so it filters out failed casts automatically.
	
	TArray<UObject*> CastResult = Linq::Start(ObjectList)
		.IsValid()
		.Cast<UObject>() // Trivial cast
		.ToArray();


	TestEqual("Cast<UObject> should keep valid objects", CastResult.Num(), 2);

	// Clean up (GC usually handles this, but good practice in tests if creating Root objects)
	ValidObj = nullptr;

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqBigIntegerArrayOperation, "Logicraft.Linq.BigIntegerArrayOperation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinqBigIntegerArrayOperation::RunTest(const FString& Parameters)
{
	const int32 ElementCount = 1000000; // 1 Million d'éléments
    TArray<int64> BigArray;
    BigArray.Reserve(ElementCount);

    // Remplissage avec des valeurs pseudo-aléatoires
    for (int32 i = 0; i < ElementCount; ++i)
    {
        BigArray.Add(FMath::RandRange(0, 10000));
    }

    UE_LOG(LogTemp, Display, TEXT("--- START BENCHMARK (N=%d) ---"), ElementCount);

    // ==========================================
    // TEST 1: AGGREGATION (Where + Sum)
    // ==========================================
    {
        // --- BASELINE (Raw Loop) ---
        double StartRaw = FPlatformTime::Seconds();
        int64 SumRaw = 0;
        for (int32 Val : BigArray)
        {
            if (Val % 2 == 0) // Nombres pairs uniquement
            {
                SumRaw += Val;
            }
        }
        double DurationRaw = (FPlatformTime::Seconds() - StartRaw) * 1000.0;

        // --- LINQ ---
        double StartLinq = FPlatformTime::Seconds();
        int64 SumLinq = Linq::Start(BigArray)
            .Where([](int32 Val) { return Val % 2 == 0; })
            .Sum(); // On somme en int64 pour éviter l'overflow
        double DurationLinq = (FPlatformTime::Seconds() - StartLinq) * 1000.0;

        // Validation et Log
        TestEqual(TEXT("Sum Results match"), SumLinq, SumRaw);
        UE_LOG(LogTemp, Display, TEXT("[Sum] Raw: %.2f ms | LINQ: %.2f ms | Overhead: %.2fx"), DurationRaw, DurationLinq, DurationLinq / DurationRaw);
    }

    // ==========================================
    // TEST 2: ALLOCATION (Select + ToArray)
    // ==========================================
    {
        // --- BASELINE (Raw Loop) ---
        double StartRaw = FPlatformTime::Seconds();
        TArray<FString> StrArrayRaw;
        StrArrayRaw.Reserve(ElementCount); // Optimisation manuelle souvent oubliée en LINQ
        for (int32 Val : BigArray)
        {
            StrArrayRaw.Add(FString::FromInt(Val));
        }
        double DurationRaw = (FPlatformTime::Seconds() - StartRaw) * 1000.0;

        // --- LINQ ---
        double StartLinq = FPlatformTime::Seconds();
        TArray<FString> StrArrayLinq = Linq::Start(BigArray)
            .Select([](int32 Val) { return FString::FromInt(Val); })
            .ToArray();
        double DurationLinq = (FPlatformTime::Seconds() - StartLinq) * 1000.0;


        // Validation et Log
        TestEqual(TEXT("ToArray Count matches"), StrArrayLinq.Num(), StrArrayRaw.Num());
        UE_LOG(LogTemp, Display, TEXT("[Select+ToArray] Raw: %.2f ms | LINQ: %.2f ms | Overhead: %.2fx"),
        	DurationRaw, DurationLinq, DurationLinq / DurationRaw);
    }

    // ==========================================
    // TEST 3: EARLY EXIT (First)
    // ==========================================
    {
        // On cherche une valeur qui est sûrement vers la fin ou milieu pour voir l'itération
        int64 TargetValue = 9999; 
        
        // --- BASELINE (Raw Loop) ---
        double StartRaw = FPlatformTime::Seconds();
        int32 FoundRaw = -1;
        for (int32 Val : BigArray)
        {
            if (Val == TargetValue)
            {
                FoundRaw = Val;
                break; // Break manuel indispensable
            }
        }
        double DurationRaw = (FPlatformTime::Seconds() - StartRaw) * 1000.0;

        // --- LINQ ---
        double StartLinq = FPlatformTime::Seconds();
        // First() doit s'arrêter dès qu'il trouve (Lazy Evaluation), il ne doit pas parcourir tout le tableau
        int32 FoundLinq = Linq::Start(BigArray).FirstOrDefault([TargetValue](int64 Val) { return Val == TargetValue; }, 0ll);
        double DurationLinq = (FPlatformTime::Seconds() - StartLinq) * 1000.0;

        // Validation et Log
        TestEqual(TEXT("First Result matches"), FoundLinq, FoundRaw);
        UE_LOG(LogTemp, Display, TEXT("[First] Raw: %.2f ms | LINQ: %.2f ms | Overhead: %.2fx"), DurationRaw, DurationLinq, DurationLinq / DurationRaw);
    }
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqMultiContainerSupport, "Logicraft.Linq.MultiContainerSupport", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinqMultiContainerSupport::RunTest(const FString& Parameters)
{
	TArray<FString> Users = {
		{TEXT("Alice")},
		{TEXT("Bob")},
		{TEXT("Charlie")},
		{TEXT("David")},
		{TEXT("Eve")},
		{TEXT("Frank")},
		{TEXT("Grace")},
		{TEXT("Hannah")},
		{TEXT("Isaac")},
		{TEXT("Jack")}
	};

	auto Map = Linq::Start(Users)
		.Where([](const FString& User)
		{
			return User.Len() > 3;
		})
		.ToMap(
		[](const FString& User)
		{
			return User;
		},
		[Id = 0] (const FString& User) mutable{
			return Id++;
		});

	auto SameMap = Linq::Start(Map)
		.ToMap();
	
	auto Set = Linq::Start(Map)
		.Take(5)
		.ToSet();
	
	
	return true;
}
