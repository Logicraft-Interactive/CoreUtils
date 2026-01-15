#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Linq.h"
#include "LinqCustomObject.h"
#include "UObject/Package.h"

// Define a simple test category
DEFINE_LOG_CATEGORY_STATIC(LogLinqTest, Log, All);

// 1. Basic Integer Tests: Filtering, Projection, Aggregation
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqIntegerTest, "Project.Linq.IntegerLogic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
 

bool FLinqIntegerTest::RunTest(const FString& Parameters)
{
	// Setup
	TArray<int32> Numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	// Test: Where (Filter even numbers)
	TArray<int32> EvenNumbers = Linq::StartCustom(Numbers)
		.Where([](int32 N) { return N % 2 == 0; })
		.ToArray();
 
	
	TestEqual("Where should filter even numbers", EvenNumbers.Num(), 5);
	TestEqual("First even number should be 2", EvenNumbers[0], 2);

	// Test: Select (Square numbers)
	TArray<int32> Squared = Linq::StartCustom(Numbers)
		.Take(3)
		.Select([](int32 N) { return N * N; })
		.ToArray();

	TestEqual("Select should return 3 items due to Take(3)", Squared.Num(), 3);
	TestEqual("First square should be 1", Squared[0], 1);
	TestEqual("Third square should be 9", Squared[2], 9);

	// Test: Sum
	int32 SumOfFirstFive = Linq::StartCustom(Numbers)
		.Take(5)
		.Sum();
	
	TestEqual("Sum of 1 to 5 should be 15", SumOfFirstFive, 15);

	// Test: Count
	int32 Count = Linq::StartCustom(Numbers)
	.Take(4)
	.Count([](int32 N){return N % 2 == 0;});

	TestEqual("Count of even number from 1 to 4 should return 2", Count, 2);
	
	// Test: Any / All
	bool bHasNegative = Linq::StartCustom(Numbers).Any([](int32 N) { return N < 0; });
	bool bAllPositive = Linq::StartCustom(Numbers).All([](int32 N) { return N > 0; });

	TestFalse("Collection should not have negative numbers", bHasNegative);
	TestTrue("Collection should have all positive numbers", bAllPositive);

	return true;
}

// 2. Complex Object Tests: Sorting and Edge Cases
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqSortingTest, "Project.Linq.Sorting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinqSortingTest::RunTest(const FString& Parameters)
{
	TArray<int32> Unsorted = { 5, 1, 9, 3 };

	// Test: OrderBy (Ascending)
	TArray<int32> Sorted = Linq::StartCustom(Unsorted)
		.OrderBy()
		.ToArray();

	TestEqual("First item should be 1", Sorted[0], 1);
	TestEqual("Last item should be 9", Sorted.Last(), 9);

	// Test: OrderBy (Descending via Comparator)
	TArray<int32> Descending = Linq::StartCustom(Unsorted)
		.OrderBy([](int32 A, int32 B) { return A > B; }) // Inverse comparator
		.ToArray();

	TestEqual("First item should be 9", Descending[0], 9);
	
	// Test: First / FirstOrDefault
	int32 FirstMatch = Linq::StartCustom(Unsorted).First([](int32 N) { return N > 4; });
	TestEqual("First number > 4 should be 5", FirstMatch, 5);

	int32 Missing = Linq::StartCustom(Unsorted).FirstOrDefault([](int32 N) { return N > 100; }, -1);
	TestEqual("Should return default value -1 when not found", Missing, -1);

	return true;
}

// 3. Unreal Engine Specific Tests: UObjects, Cast, IsValid
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinqUObjectTest, "Project.Linq.UObjectFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinqUObjectTest::RunTest(const FString& Parameters)
{
	// Setup: Create a transient package to hold objects
	UPackage* Package = GetTransientPackage();
	
	// Create mixed objects
	UObject* ValidObj = NewObject<ULinqCustomObject>(Package);
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
	TArray<UObject*> ValidOnly = Linq::StartCustom(ObjectList)
		.IsValid()
		.ToArray();

	TestEqual("IsValid should remove nullptr", ValidOnly.Num(), 2);

	// Test: Apply (Side Effects)
	// We will use a counter to verify Apply was called on valid objects
	int32 CallCount = 0;
	Linq::StartCustom(ValidOnly)
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
	
	TArray<UObject*> CastResult = Linq::StartCustom(ObjectList)
		.IsValid()
		.Cast<UObject>() // Trivial cast
		.ToArray();

	TestEqual("Cast<UObject> should keep valid objects", CastResult.Num(), 2);

	// Clean up (GC usually handles this, but good practice in tests if creating Root objects)
	ValidObj = nullptr;

	return true;
}