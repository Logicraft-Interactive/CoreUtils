// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/SlateFontStyleHelper.h"
#include "LogCategory.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

FSlateFontInfo SlateFontStyleHelper::GetFontStyle(FStringView FontPath, float FontSize)
{
	return FSlateFontInfo{ FontPath.GetData(), FontSize };
}

FSlateFontInfo SlateFontStyleHelper::GetFontStyleSafe(const FString& FontPath, float FontSize)
{
	if (FPaths::FileExists(FontPath))
	{
		return FSlateFontInfo{ FontPath, FontSize };
	}

	UE_LOG(LogLCU, Error,
		TEXT("SlateFontStyleHelper: Unable to load the wanted font [Font Path: %s].\n"
					 "Loading default font 'Roboto-Regular' instead."), *FontPath)
	
	return FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
}

FSlateFontInfo SlateFontStyleHelper::GetSlateFontStyle(FStringView FontName, float FontSize)
{
	const FString FontPath
	{
		FString::Printf(TEXT("%s/%s"), *(FPaths::EngineContentDir() / "Slate/Fonts"), FontName.GetData())
	};
	
	return FSlateFontInfo{ FontPath, FontSize };
}

FSlateFontInfo SlateFontStyleHelper::GetSlateFontStyleSafe(FStringView FontName, float FontSize)
{
	const FString FontPath
	{
		FString::Printf(TEXT("%s/%s"), *(FPaths::EngineContentDir() / "Slate/Fonts"), FontName.GetData())
	};

	if (FPaths::FileExists(FontPath))
	{
		return FSlateFontInfo{ FontPath, FontSize };
	}
	
	UE_LOG(LogLCU, Error,
		TEXT("SlateFontStyleHelper: Unable to load the wanted font [Font Path: %s].\n"
					 "Loading default font 'Roboto-Regular' instead."), *FontPath)
	
	return FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
}
