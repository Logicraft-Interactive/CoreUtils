// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "SaveSystem/SaveSubsystem.h"
#include "LogCategory.h"

void USaveSubsystem::SaveWorld(const FName& SlotName, const FString& Version)
{
	TOptional<VersionType> OptionalVersionTuple = ExtractVersion(Version);

	if (!OptionalVersionTuple.IsSet())
	{
		UE_LOG(LogSaveSystem, Error, TEXT("Can't continue saving with invalid version format"));
		return;
	}

	CurrentSave.ObjectsData.Empty();
	CurrentSave.SaveSlotName = SlotName;
	CurrentSave.SaveTimeStamp = FDateTime::Now();
	CurrentSave.GlobalSaveVersion = Version;
	
}

FObjectSaveData FSaveSerializer::SerializeObject(UObject* Object)
{
}

TOptional<USaveSubsystem::VersionType> USaveSubsystem::ExtractVersion(const FString& Version)
{
	int MajorVersion{1}, MinorVersion{0}, PatchVersion{0};

	const FRegexPattern Pattern(TEXT("^(\\d+)\\.(\\d+)\\.(\\d+)$"));
    
	FRegexMatcher Matcher(Pattern, Version);

	if (Matcher.FindNext())
	{
		// Extract groups. Index 1 is the first group, 2 is the second, etc.
		// Convert the captured string to an integer
		MajorVersion = FCString::Atoi(*Matcher.GetCaptureGroup(1));
		MinorVersion = FCString::Atoi(*Matcher.GetCaptureGroup(2));
		PatchVersion = FCString::Atoi(*Matcher.GetCaptureGroup(3));

		// Log for debugging
		UE_LOG(LogSaveSystem, Log, TEXT("Parsed Version: %d.%d.%d"), MajorVersion, MinorVersion, PatchVersion);
	}
	else
	{
		UE_LOG(LogSaveSystem, Warning, TEXT("Invalid version format: %s"), *Version);
		return NullOpt;
	}

	return VersionType(MajorVersion, MinorVersion, PatchVersion);
}
