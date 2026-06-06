// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

/**
 * Utility class for loading fonts in packaged builds.
 * 
 * Problem: FAppStyle::GetFontStyle() is editor-only and crashes in packaged builds.
 * Solution: This helper provides safe alternatives that work in both editor and packaged builds.
 * 
 * Available Roboto fonts (shipped with Unreal Engine):
 * - Roboto-Regular.ttf
 * - Roboto-Bold.ttf
 * - Roboto-Italic.ttf
 * - Roboto-BoldItalic.ttf
 * - Roboto-Light.ttf
 * - Roboto-Medium.ttf
 * 
 * All methods are static and can be called from anywhere.
 * 
 * Thread Safety: Safe to call from any thread (read-only operations).
 */
class LOGICRAFTCOREUTILS_API SlateFontStyleHelper
{
public:
	/**
	 * Loads a font from an absolute file path.
	 * 
	 * WARNING: No validation - will crash if the path is invalid!
	 * Use GetFontStyleSafe() for production code.
	 * 
	 * @param FontPath - Full path to the .ttf file (e.g., "C:/MyProject/Fonts/MyFont.ttf")
	 * @param FontSize - Font size in points
	 * @return Font information ready to use in Slate widgets
	 * 
	 * Example:
	 * @code
	 * FSlateFontInfo Font = SlateFontStyleHelper::GetFontStyle(
	 *     TEXT("D:/MyFonts/CustomFont.ttf"), 16);
	 * @endcode
	 */
	static FSlateFontInfo GetFontStyle(FStringView FontPath, float FontSize);

	/**
	 * Loads a font from an absolute file path with safety validation.
	 * 
	 * If the font file doesn't exist, falls back to Roboto-Regular.
	 * 
	 * @param FontPath - Full path to the .ttf file
	 * @param FontSize - Font size in points
	 * @return Font information (or fallback if path is invalid)
	 * 
	 * Example:
	 * @code
	 * FSlateFontInfo Font = SlateFontStyleHelper::GetFontStyleSafe(
	 *     TEXT("D:/MyFonts/MightNotExist.ttf"), 16);
	 * // If file doesn't exist, returns Roboto-Regular instead of crashing
	 * @endcode
	 */
	static FSlateFontInfo GetFontStyleSafe(const FString& FontPath, float FontSize);

	/**
	 * Loads a Roboto font from Unreal's Engine/Content/Slate/Fonts directory.
	 * 
	 * WARNING: No validation - will crash if the font name is wrong!
	 * Use GetSlateFontStyleSafe() for production code.
	 * 
	 * Available fonts:
	 * - Roboto-Regular.ttf
	 * - Roboto-Bold.ttf
	 * - Roboto-Italic.ttf
	 * - Roboto-BoldItalic.ttf
	 * - Roboto-Light.ttf
	 * - Roboto-Medium.ttf
	 * 
	 * @param FontName - Just the filename (e.g., "Roboto-Bold.ttf")
	 * @param FontSize - Font size in points
	 * @return Font information ready to use
	 * 
	 * Example:
	 * @code
	 * FSlateFontInfo Font = SlateFontStyleHelper::GetSlateFontStyle(
	 *     TEXT("Roboto-Bold.ttf"), 18);
	 * @endcode
	 */
	static FSlateFontInfo GetSlateFontStyle(FStringView FontName, float FontSize);
	
	/**
	 * Loads a Roboto font from Unreal's Slate/Fonts directory with safety validation.
	 * 
	 * If the font file doesn't exist, falls back to Roboto-Regular.
	 * This is the RECOMMENDED method for production code.
	 * 
	 * @param FontName - Just the filename (e.g., "Roboto-Bold.ttf")
	 * @param FontSize - Font size in points
	 * @return Font information (or fallback if font name is invalid)
	 * 
	 * Example:
	 * @code
	 * FSlateFontInfo CategoryFont = SlateFontStyleHelper::GetSlateFontStyleSafe(
	 *     TEXT("Roboto-Bold.ttf"), 16);
	 * 
	 * SNew(STextBlock)
	 *     .Font(CategoryFont)
	 *     .Text(FText::FromString("Category"));
	 * @endcode
	 */
	static FSlateFontInfo GetSlateFontStyleSafe(FStringView FontName, float FontSize);
};