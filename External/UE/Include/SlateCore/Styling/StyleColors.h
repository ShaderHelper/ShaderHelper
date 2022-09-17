// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateColor.h"
#include "StyleColors.generated.h"

/** Themes are only allowed in the editor or standalone tools */
#define ALLOW_THEMES WITH_EDITOR || IS_PROGRAM

// HEX Colors from sRGB Color space
#define COLOR( HexValue ) FLinearColor::FromSRGBColor(FColor::FromHex(HexValue))

/**
 * Note: If you add another color here, you should update the Dark.json theme file in Engine\Content\Slate\Themes & FUMGColors in UMGCoreStyle.cpp for consistency
 */
UENUM()
enum class EStyleColor : uint8
{
	Black,
	Background,
	Title,
	WindowBorder,
	Foldout,
	Input,
	InputOutline,
	Recessed,
	Panel,
	Header,
	Dropdown,
	DropdownOutline,
	Hover,
	Hover2,
	White,
	White25,
	Highlight,
	Primary,
	PrimaryHover,
	PrimaryPress,
	Secondary, 
	Foreground,
	ForegroundHover,
	ForegroundInverted,
	ForegroundHeader,
	Select,
	SelectInactive,
	SelectParent,
	SelectHover,
	Notifications,
	AccentBlue,
	AccentPurple,
	AccentPink,
	AccentRed,
	AccentOrange,
	AccentYellow,
	AccentGreen,
	AccentBrown,
	AccentBlack,
	AccentGray,
	AccentWhite,
	AccentFolder,
	Warning,
	Error,
	Success,

	/** Only user colors should be below this line
	 * To use user colors:
	 * 1. Set an unused user enum value below as the color value for an FSlateColor. E.g. FSlateColor MyCustomColor(EStyleColors::User1)
	 * 2. Set the actual color. E.g USlateThemeManager::Get().SetDefaultColor(EStyleColor::User1, FLinearColor::White)
	 * 3. Give it a display name if you want it to be configurable by editor users. E.g.  UStyleColorTable::Get().SetColorDisplayName(EUserStyleColor::User1, "My Color Name")
	 */
	User1,
	User2,
	User3,
	User4,
	User5,
	User6,
	User7,
	User8,
	User9,
	User10,
	User11,
	User12,
	User13,
	User14,
	User15,
	User16,

	MAX
};

USTRUCT()
struct FStyleColorList
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = Colors)
	FLinearColor StyleColors[(int32)EStyleColor::MAX] = {FLinearColor::Transparent};

	FText DisplayNames[(int32)EStyleColor::MAX];
};

/**
 * Represents a single theme
 */
USTRUCT()
struct FStyleTheme
{
	GENERATED_BODY()

	/** Unique Id for the theme */
	FGuid Id;
	/** Friendly, user customizable theme name */
	FText DisplayName;
	/** Filename where the theme is stored */
	FString Filename;
	/** The default colors for this theme. Used for resetting to default. Not the active colors*/
	TArray<FLinearColor> LoadedDefaultColors;

	bool operator==(const FStyleTheme& Other) const
	{
		return Id == Other.Id;
	}

	bool operator==(const FGuid& OtherId) const
	{
		return Id == OtherId;
	}
};

UCLASS(Config=EditorSettings)
class SLATECORE_API USlateThemeManager : public UObject 
{
	GENERATED_BODY()
public:
	static USlateThemeManager& Get()
	{
		return *GetMutableDefault<USlateThemeManager>();
	}

	const FLinearColor& GetColor(EStyleColor Color)
	{
		return ActiveColors.StyleColors[static_cast<int32>(Color)];
	}


	USlateThemeManager();

	/**
	 * Initializes default colors
	 */
	void InitalizeDefaults();

	/**
	 * Sets a default color to be used as a fallback if no theme is loaded
	 */
	void SetDefaultColor(EStyleColor InColorId, FLinearColor InColor)
	{
#if ALLOW_THEMES
		DefaultColors[static_cast<int32>(InColorId)] = InColor;
#else
		ActiveColors.StyleColors[static_cast<int32>(InColorId)] = InColor;
#endif
	}

#if ALLOW_THEMES

	/** Sets a custom display name for a style color */
	void SetColorDisplayName(EStyleColor InColorId, FText DisplayName)
	{
		ActiveColors.DisplayNames[static_cast<int32>(InColorId)] = DisplayName;
	}

	/** Gets a custom display name for a style color. This will be empty if no custom name was chosen */
	FText GetColorDisplayName(EStyleColor InColorId) const
	{
		return ActiveColors.DisplayNames[static_cast<int32>(InColorId)];
	}

	/**
	 * Load all known themes from engine, project, and user directories
	 */
	void LoadThemes();

	/**
	 * Saves the current theme
	 * 
	 * @param The filename to save the current theme as
	 */
	void SaveCurrentThemeAs(const FString& Filename);

	/**
	 * Applies a theme as the active theme
	 */
	void ApplyTheme(FGuid ThemeId);

	/**
	 * Removes a theme. 
	 * Note: The active theme cannot be removed and there must always be an active theme.  Apply a new theme first before removing the current theme.
	 */
	void RemoveTheme(FGuid ThemeId);

	/**
	 * Duplicates the active theme
	 * 
	 * @return the id of the new theme
	 */
	FGuid DuplicateActiveTheme();

	/**
	 * Sets the display name for the current theme
	 */
	void SetCurrentThemeDisplayName(FText NewDisplayName);

	/**
	 * @return the current theme
	 */
	const FStyleTheme& GetCurrentTheme() const { return *Themes.FindByKey(CurrentThemeId); }

	/**
	 * @return All known themes
	 */
	const TArray<FStyleTheme>& GetThemes() const { return Themes; }

	/**
	 * Resets an active color to the default color for the curerent theme
	 */
	void ResetActiveColorToDefault(EStyleColor Color);

	/**
	 * Validate that there is an active loaded theme
	 */
	void ValidateActiveTheme();

	/**
	 * @return the engine theme dir.  Engine themes are project agnostic
	 */
	FString GetEngineThemeDir() const;

	/**
	 * @return the project theme dir. Project themes can override engine themes
	 */
	FString GetProjectThemeDir() const;

	/**
	 * @return the user theme dir. Themes in this dir are per-user and override engine and project themes
	 */
	FString GetUserThemeDir() const;
private:

	FStyleTheme& GetMutableCurrentTheme() { return *Themes.FindByKey(CurrentThemeId); }
	void LoadThemesFromDirectory(const FString& Directory);
	bool ReadTheme(const FString& ThemeData, FStyleTheme& OutTheme);
	void EnsureValidCurrentTheme();
	void LoadThemeColors(FStyleTheme& Theme);


private:
	TArray<FStyleTheme> Themes;
	FLinearColor DefaultColors[(int32)EStyleColor::MAX];
#endif // ALLOW_THEMES

	FLinearColor GetDefaultColor(EStyleColor InColorId)
	{
#if ALLOW_THEMES
		return DefaultColors[static_cast<int32>(InColorId)];
#else
		return ActiveColors.StyleColors[static_cast<int32>(InColorId)];
#endif
	}

	UPROPERTY(EditAnywhere, Config, Category=Colors)
	FGuid CurrentThemeId;

	UPROPERTY(EditAnywhere, transient, Category = Colors)
	FStyleColorList ActiveColors;
};

/**
 * Common/themeable colors used by all styles
 * Please avoid adding new generic colors to this list without discussion first
 */
struct SLATECORE_API FStyleColors
{
	static const FSlateColor Transparent;
	static const FSlateColor Black;
	static const FSlateColor Title;
	static const FSlateColor WindowBorder;
	static const FSlateColor Foldout;
	static const FSlateColor Input;
	static const FSlateColor InputOutline;
	static const FSlateColor Recessed;
	static const FSlateColor Background;
	static const FSlateColor Panel;
	static const FSlateColor Header;
	static const FSlateColor Dropdown;
	static const FSlateColor DropdownOutline;
	static const FSlateColor Hover;
	static const FSlateColor Hover2;
	static const FSlateColor White;
	static const FSlateColor White25;
	static const FSlateColor Highlight;

	static const FSlateColor Primary;
	static const FSlateColor PrimaryHover;
	static const FSlateColor PrimaryPress;
	static const FSlateColor Secondary;

	static const FSlateColor Foreground;
	static const FSlateColor ForegroundHover;
	static const FSlateColor ForegroundInverted;
	static const FSlateColor ForegroundHeader;

	static const FSlateColor Select;
	static const FSlateColor SelectInactive;
	static const FSlateColor SelectParent;
	static const FSlateColor SelectHover;

	static const FSlateColor Notifications;

	static const FSlateColor AccentBlue;
	static const FSlateColor AccentPurple;
	static const FSlateColor AccentPink;
	static const FSlateColor AccentRed;
	static const FSlateColor AccentOrange;
	static const FSlateColor AccentYellow;
	static const FSlateColor AccentGreen;
	static const FSlateColor AccentBrown;
	static const FSlateColor AccentBlack;
	static const FSlateColor AccentGray;
	static const FSlateColor AccentWhite;
	static const FSlateColor AccentFolder;

	static const FSlateColor Warning;
	static const FSlateColor Error;
	static const FSlateColor Success;
};
