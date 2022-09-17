// Copyright Epic Games, Inc. All Rights Reserved.

/*-----------------------------------------------------------------------------
	Config cache.
-----------------------------------------------------------------------------*/

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "Containers/StringFwd.h"
#include "Math/Color.h"
#include "UObject/NameTypes.h"
#include "Logging/LogMacros.h"
#include "Math/Vector2D.h"
#include "Delegates/Delegate.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Misc/AccessDetection.h"
#include "Misc/Paths.h"
#include "Serialization/StructuredArchive.h"

CORE_API DECLARE_LOG_CATEGORY_EXTERN(LogConfig, Log, All);

// Server builds should be tweakable even in Shipping
#define ALLOW_INI_OVERRIDE_FROM_COMMANDLINE			(UE_SERVER || !(UE_BUILD_SHIPPING))
#define CONFIG_REMEMBER_ACCESS_PATTERN (WITH_EDITOR || 0)


///////////////////////////////////////////////////////////////////////////////
//
// This is the master list of known ini files that are used and processed 
// on all platforms (specifically for runtime/binary speedups. Other, editor-
// specific inis, or non-hierarchical ini files will still work with the 
// old system, but they won't have any optimizations applied
//
// These should be listed in the order of highest to lowest use, for optimization
//
///////////////////////////////////////////////////////////////////////////////

#define ENUMERATE_KNOWN_INI_FILES(op) \
	op(Engine) \
	op(Game) \
	op(Input) \
	op(DeviceProfiles) \
	op(GameUserSettings) \
	op(Scalability) \
	op(RuntimeOptions) \
	op(InstallBundle) \
	op(Hardware) \
	op(GameplayTags)


#define KNOWN_INI_ENUM(IniName) IniName,
enum class EKnownIniFile : uint8
{
	// make an enum entry for each known ini above
	ENUMERATE_KNOWN_INI_FILES(KNOWN_INI_ENUM)
	
	// conventient counter for the above list
	NumKnownFiles,
};

namespace UE
{
namespace ConfigCacheIni
{
namespace Private
{
struct FAccessor;
}
}
}


struct FConfigValue
{
public:
	FConfigValue() { }

	FConfigValue(const TCHAR* InValue)
		: SavedValue(InValue)
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		, bRead(false)
#endif
	{
		ExpandValueInternal();
	}

	FConfigValue(const FString& InValue)
		: SavedValue(InValue)
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		, bRead(false)
#endif
	{
		ExpandValueInternal();
	}

	FConfigValue(FString&& InValue)
		: SavedValue(MoveTemp(InValue))
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		, bRead(false)
#endif
	{
		ExpandValueInternal();
	}

	FConfigValue(const FConfigValue& InConfigValue)
		: SavedValue(InConfigValue.SavedValue)
		, ExpandedValue(InConfigValue.ExpandedValue)
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		, bRead(InConfigValue.bRead)
#endif
	{
		// shouldn't need to expand value it's assumed that the other FConfigValue has done this already
	}

	FConfigValue(FConfigValue&& InConfigValue)
		: SavedValue(MoveTemp(InConfigValue.SavedValue))
		, ExpandedValue(MoveTemp(InConfigValue.ExpandedValue))
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		, bRead(InConfigValue.bRead)
#endif
	{
		// shouldn't need to expand value it's assumed that the other FConfigValue has done this already
	}

	FConfigValue& operator=(FConfigValue&& RHS)
	{
		SavedValue = MoveTemp(RHS.SavedValue);
		ExpandedValue = MoveTemp(RHS.ExpandedValue);
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		bRead = RHS.bRead;
#endif

		return *this;
	}

	FConfigValue& operator=(const FConfigValue& RHS)
	{
		SavedValue = RHS.SavedValue;
		ExpandedValue = RHS.ExpandedValue;
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		bRead = RHS.bRead;
#endif

		return *this;
	}

	// Returns the ini setting with any macros expanded out
	const FString& GetValue() const 
	{
		UE::AccessDetection::ReportAccess(UE::AccessDetection::EType::Ini);
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		bRead = true; 
#endif
		return (ExpandedValue.Len() > 0 ? ExpandedValue : SavedValue); 
	}

	// Returns the original ini setting without macro expansion
	const FString& GetSavedValue() const 
	{
		UE::AccessDetection::ReportAccess(UE::AccessDetection::EType::Ini);
#if CONFIG_REMEMBER_ACCESS_PATTERN 
		bRead = true; 
#endif
		return SavedValue; 
	}
#if CONFIG_REMEMBER_ACCESS_PATTERN 
	inline const bool HasBeenRead() const
	{
		return bRead;
	}
	inline void SetHasBeenRead(bool InBRead ) const
	{
		bRead = InBRead;
	}
#endif

	bool operator==(const FConfigValue& Other) const { return (SavedValue.Compare(Other.SavedValue) == 0); }
	bool operator!=(const FConfigValue& Other) const { return !(FConfigValue::operator==(Other)); }

	friend FArchive& operator<<(FArchive& Ar, FConfigValue& ConfigSection)
	{
		FStructuredArchiveFromArchive(Ar).GetSlot() << ConfigSection;
		return Ar;
	}

	friend void operator<<(FStructuredArchive::FSlot Slot, FConfigValue& ConfigSection)
	{
		Slot << ConfigSection.SavedValue;

		if (Slot.GetUnderlyingArchive().IsLoading())
		{
			ConfigSection.ExpandValueInternal();
#if CONFIG_REMEMBER_ACCESS_PATTERN 
			ConfigSection.bRead = false;
#endif
		}
	}

	/**
	 * Given a collapsed config value, try and produce an expanded version of it (removing any placeholder tokens).
	 *
	 * @param InCollapsedValue		The collapsed config value to try and expand.
	 * @param OutExpandedValue		String to fill with the expanded version of the config value.
	 *
	 * @return true if expansion occurred, false if the collapsed and expanded values are equal.
	 */
	CORE_API static bool ExpandValue(const FString& InCollapsedValue, FString& OutExpandedValue);

	/**
	 * Given a collapsed config value, try and produce an expanded version of it (removing any placeholder tokens).
	 *
	 * @param InCollapsedValue		The collapsed config value to try and expand.
	 *
	 * @return The expanded version of the config value.
	 */
	CORE_API static FString ExpandValue(const FString& InCollapsedValue);

	/**
	 * Given an expanded config value, try and produce a collapsed version of it (adding any placeholder tokens).
	 *
	 * @param InExpandedValue		The expanded config value to try and expand.
	 * @param OutCollapsedValue		String to fill with the collapsed version of the config value.
	 *
	 * @return true if collapsing occurred, false if the collapsed and expanded values are equal.
	 */
	CORE_API static bool CollapseValue(const FString& InExpandedValue, FString& OutCollapsedValue);

	/**
	 * Given an expanded config value, try and produce a collapsed version of it (adding any placeholder tokens).
	 *
	 * @param InExpandedValue		The expanded config value to try and expand.
	 *
	 * @return The collapsed version of the config value.
	 */
	CORE_API static FString CollapseValue(const FString& InExpandedValue);

private:
	/** Internal version of ExpandValue that expands SavedValue into ExpandedValue, or produces an empty ExpandedValue if no expansion occurred. */
	CORE_API void ExpandValueInternal();

	/** Gets the SavedValue without marking it as having been accessed for e.g. writing out to a ConfigFile to disk */
	friend struct UE::ConfigCacheIni::Private::FAccessor;
	const FString& GetSavedValueForWriting() const
	{
		return SavedValue;
	};

	FString SavedValue;
	FString ExpandedValue;
#if CONFIG_REMEMBER_ACCESS_PATTERN 
	mutable bool bRead; // has this value been read since the config system started
#endif
};

namespace UE
{
namespace ConfigCacheIni
{
namespace Private
{
/** An accessor class to access functions that should be restricted only to FConfigFileCache Internal use */
struct FAccessor
{
private:
	friend class ::FConfigCacheIni;
	friend class ::FConfigFile;

	static const FString& GetSavedValueForWriting(const FConfigValue& ConfigValue)
	{
		return ConfigValue.GetSavedValueForWriting();
	}
};
}
}
}

typedef TMultiMap<FName,FConfigValue> FConfigSectionMap;

// One section in a config file.
class FConfigSection : public FConfigSectionMap
{
public:
	/**
	* Check whether the input string is surrounded by quotes
	*
	* @param Test The string to check
	*
	* @return true if the input string is surrounded by quotes
	*/
	static bool HasQuotes( const FString& Test );
	bool operator==( const FConfigSection& Other ) const;
	bool operator!=( const FConfigSection& Other ) const;

	// process the '+' and '.' commands, takingf into account ArrayOfStruct unique keys
	void CORE_API HandleAddCommand(FName Key, FString&& Value, bool bAppendValueIfNotArrayOfStructsKeyUsed);

	bool HandleArrayOfKeyedStructsCommand(FName Key, FString&& Value);

	template<typename Allocator> 
	void MultiFind(const FName Key, TArray<FConfigValue, Allocator>& OutValues, const bool bMaintainOrder = false) const
	{
		FConfigSectionMap::MultiFind(Key, OutValues, bMaintainOrder);
	}

	template<typename Allocator> 
	void MultiFind(const FName Key, TArray<FString, Allocator>& OutValues, const bool bMaintainOrder = false) const
	{
		for (typename ElementSetType::TConstKeyIterator It(Pairs, Key); It; ++It)
		{
			OutValues.Add(It->Value.GetValue());
		}

		if (bMaintainOrder)
		{
			Algo::Reverse(OutValues);
		}
	}

	// look for "array of struct" keys for overwriting single entries of an array
	TMap<FName, FString> ArrayOfStructKeys;

	friend FArchive& operator<<(FArchive& Ar, FConfigSection& ConfigSection);
};

FArchive& operator<<(FArchive& Ar, FConfigSection& ConfigSection);

/**
 * FIniFilename struct.
 * 
 * Helper struct for generating ini files.
 */
struct FIniFilename
{
	/** Ini filename */
	FString Filename;
	/** If true this ini file is required to generate the output ini. */
	bool bRequired = false;
	/** Used as ID for looking up an INI Hierarchy */
	FString CacheKey;

	explicit FIniFilename(const FString& InFilename, bool InIsRequired=false, FString InCacheKey=FString(TEXT("")))
		: Filename(InFilename)
		, bRequired(InIsRequired) 
		, CacheKey(InCacheKey)
	{}

	FIniFilename() = default;

	friend FArchive& operator<<(FArchive& Ar, FIniFilename& IniFilename)
	{
		Ar << IniFilename.Filename;
		Ar << IniFilename.bRequired;
		Ar << IniFilename.CacheKey;
		return Ar;
	}
};


#if ALLOW_INI_OVERRIDE_FROM_COMMANDLINE
// Options which stemmed from the commandline
struct FConfigCommandlineOverride
{
	FString BaseFileName, Section, PropertyKey, PropertyValue;
};
#endif // ALLOW_INI_OVERRIDE_FROM_COMMANDLINE


class FConfigFileHierarchy : public TMap<int32, FIniFilename>
{
private:
	int32 KeyGen = 0;

public:
	FConfigFileHierarchy();

	friend FArchive& operator<<(FArchive& Ar, FConfigFileHierarchy& ConfigFileHierarchy)
	{
		Ar << static_cast<FConfigFileHierarchy::Super&>(ConfigFileHierarchy);
		Ar << ConfigFileHierarchy.KeyGen;
		return Ar;
	}

private:
	int32 GenerateDynamicKey();

	int32 AddStaticLayer(FIniFilename Filename, int32 LayerIndex, int32 ExpansionIndex=0, int32 PlatformIndex=0);
	int32 AddDynamicLayer(FIniFilename Filename);

	friend class FConfigFile;
};

// One config file.

class FConfigFile : public TMap<FString,FConfigSection>
{
public:
	bool Dirty;
	bool NoSave;
	bool bHasPlatformName = false;

	/** The name of this config file */	
	FName Name;

	// The collection of source files which were used to generate this file.
	FConfigFileHierarchy SourceIniHierarchy;

	// Locations where this file may have come from - used to merge with non-standard ini locations
	FString SourceEngineConfigDir;
	FString SourceProjectConfigDir;

	/** The untainted config file which contains the coalesced base/default options. I.e. No Saved/ options*/
	FConfigFile* SourceConfigFile;

	/** Key to the cache to speed up ini parsing */
	FString CacheKey;

	FString PlatformName;

#if ALLOW_INI_OVERRIDE_FROM_COMMANDLINE
	/** The collection of overrides which stemmed from the commandline */
	TArray<FConfigCommandlineOverride> CommandlineOptions;
#endif // ALLOW_INI_OVERRIDE_FROM_COMMANDLINE
	
	CORE_API FConfigFile();
	FConfigFile( int32 ) {}	// @todo UE-DLL: Workaround for instantiated TMap template during DLLExport (TMap::FindRef)
	CORE_API ~FConfigFile();
	
	// looks for a section by name, and creates an empty one if it can't be found
	CORE_API FConfigSection* FindOrAddSection(const FString& Name);

	bool operator==( const FConfigFile& Other ) const;
	bool operator!=( const FConfigFile& Other ) const;

	CORE_API bool Combine( const FString& Filename);
	CORE_API void CombineFromBuffer(const FString& Buffer);
	CORE_API void Read( const FString& Filename );

	/** Whether to write a temp file then move it to it's destination when saving. */
	CORE_API static bool WriteTempFileThenMove();

	/** Write this ConfigFile to the given Filename, constructed the text from the config sections in *this, prepended by the optional PrefixText */
	CORE_API bool Write( const FString& Filename, bool bDoRemoteWrite=true, const FString& PrefixText=FString());

	/** Write this ConfigFile to the given string, constructed the text from the config sections in *this, prepended by the optional PrefixText 
	 * @param SimulatedFilename - If writing a default hierarchal ini, can be used to correctly deduce position in the hierarchy
	 */
	CORE_API void WriteToString(FString& InOutText, const FString& SimulatedFilename = FString(), const FString& PrefixText = FString());

private:
	/** Determine if writing a default hierarchal ini, and deduce position in the hierarchy */
	bool IsADefaultIniWrite(const FString& Filename, int32& OutIniCombineThreshold) const;

	/** Write a ConfigFile to the given Filename, constructed from the given SectionTexts, in the given order, with sections in *this overriding sections in SectionTexts
	 * @param Filename - The file to write to
	 * @param bDoRemoteWrite - If true, also write the file to FRemoteConfig::Get()
	 * @param InOutSectionTexts - A map from section name to existing text for that section; text does not include the name of the section.
	 *  Entries in the TMap that also exist in *this will be updated.
	 *  If the empty string is present, it will be written out first (it is interpreted as a prefix before the first section)
	 * @param InSectionOrder - List of section names in the order in which each section should be written to disk, from e.g. the existing file.
	 *  Any section in this array that is not found in InOutSectionTexts will be ignored.
	 *  Any section in InOutSectionTexts that is not in this array will be appended to the end.
	 *  Duplicate entries are ignored; the first found index is used.
	 * @return TRUE if the write was successful
	 */
	bool WriteInternal(const FString& Filename, bool bDoRemoteWrite, TMap<FString, FString>& InOutSectionTexts, const TArray<FString>& InSectionOrder);

	/** Write a ConfigFile to InOutText, constructed from the given SectionTexts, in the given order, with sections in *this overriding sections in SectionTexts
	 * @param InOutText - The string to write to
	 * @param bIsADefaultIniWrite - If true, force all properties to be written
	 * @param IniCombineThreshold - Cutoff level for combining ini (to prevent applying changes from the same ini that we're writing)
	 * @param InOutSectionTexts - A map from section name to existing text for that section; text does not include the name of the section.
	 *  Entries in the TMap that also exist in *this will be updated.
	 *  If the empty string is present, it will be written out first (it is interpreted as a prefix before the first section)
	 * @param InSectionOrder - List of section names in the order in which each section should be written to disk, from e.g. the existing file.
	 *  Any section in this array that is not found in InOutSectionTexts will be ignored.
	 *  Any section in InOutSectionTexts that is not in this array will be appended to the end.
	 *  Duplicate entries are ignored; the first found index is used.
	 * @return TRUE if the write was successful
	 */
	void WriteToStringInternal(FString& InOutText, bool bIsADefaultIniWrite, int32 IniCombineThreshold, TMap<FString, FString>& InOutSectionTexts, const TArray<FString>& InSectionOrder);

public:
	CORE_API void Dump(FOutputDevice& Ar);

	CORE_API bool GetString( const TCHAR* Section, const TCHAR* Key, FString& Value ) const;
	CORE_API bool GetText( const TCHAR* Section, const TCHAR* Key, FText& Value ) const;
	CORE_API bool GetInt(const TCHAR* Section, const TCHAR* Key, int32& Value) const;
	CORE_API bool GetFloat(const TCHAR* Section, const TCHAR* Key, float& Value) const;
	CORE_API bool GetDouble(const TCHAR* Section, const TCHAR* Key, double& Value) const;
	CORE_API bool GetInt64( const TCHAR* Section, const TCHAR* Key, int64& Value ) const;
	CORE_API bool GetBool( const TCHAR* Section, const TCHAR* Key, bool& Value ) const;
	CORE_API int32 GetArray(const TCHAR* Section, const TCHAR* Key, TArray<FString>& Value) const;

	/* Generic versions for use with templates */
	bool GetValue(const TCHAR* Section, const TCHAR* Key, FString& Value) const
	{
		return GetString(Section, Key, Value);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, FText& Value) const
	{
		return GetText(Section, Key, Value);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, int32& Value) const
	{
		return GetInt(Section, Key, Value);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, float& Value) const
	{
		return GetFloat(Section, Key, Value);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, double& Value) const
	{
		return GetDouble(Section, Key, Value);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, int64& Value) const
	{
		return GetInt64(Section, Key, Value);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, bool& Value) const
	{
		return GetBool(Section, Key, Value);
	}
	int32 GetValue(const TCHAR* Section, const TCHAR* Key, TArray<FString>& Value) const
	{
		return GetArray(Section, Key, Value);
	}

	CORE_API void SetString(const TCHAR* Section, const TCHAR* Key, const TCHAR* Value);
	CORE_API void SetText(const TCHAR* Section, const TCHAR* Key, const FText& Value);
	CORE_API void SetFloat(const TCHAR* Section, const TCHAR* Key, float Value);
	CORE_API void SetDouble(const TCHAR* Section, const TCHAR* Key, double Value);
	CORE_API void SetBool(const TCHAR* Section, const TCHAR* Key, bool Value);
	CORE_API void SetInt64(const TCHAR* Section, const TCHAR* Key, const int64 Value);
	CORE_API void SetArray(const TCHAR* Section, const TCHAR* Key, const TArray<FString>& Value);
	
	/**
	 * Process the contents of an .ini file that has been read into an FString
	 * 
	 * @param Filename Name of the .ini file the contents came from
	 * @param Contents Contents of the .ini file
	 */
	CORE_API void ProcessInputFileContents(FStringView Contents);


	/** Adds any properties that exist in InSourceFile that this config file is missing */
	CORE_API void AddMissingProperties(const FConfigFile& InSourceFile);

	/**
	 * Saves only the sections in this FConfigFile into the given file. All other sections in the file are left alone. The sections in this
	 * file are completely replaced. If IniRootName is specified, the current section settings are diffed against the file in the hierarchy up
	 * to right before this file (so, if you are saving DefaultEngine.ini, and IniRootName is "Engine", then Base.ini and BaseEngine.ini
	 * will be loaded, and only differences against that will be saved into DefaultEngine.ini)
	 *
	 * ======================================
	 * @todo: This currently doesn't work with array properties!! It will output the entire array, and without + notation!!
	 * ======================================
	 *
	 * @param IniRootName the name (like "Engine") to use to load a .ini hierarchy to diff against
	 */
	CORE_API void UpdateSections(const TCHAR* DiskFilename, const TCHAR* IniRootName=nullptr, const TCHAR* OverridePlatform=nullptr);

	/**
	 * Update a single property in the config file, for the section that is specified.
	 */
	CORE_API bool UpdateSinglePropertyInSection(const TCHAR* DiskFilename, const TCHAR* PropertyName, const TCHAR* SectionName);

	
	/** 
	 * Check the source hierarchy which was loaded without any user changes from the Config/Saved dir.
	 * If anything in the default/base options have changed, we need to ensure that these propagate through
	 * to the final config so they are not potentially ignored
	 */
	void ProcessSourceAndCheckAgainstBackup();

	/** Checks if the PropertyValue should be exported in quotes when writing the ini to disk. */
	static bool ShouldExportQuotedString(const FString& PropertyValue);

	/** Generate a correctly escaped line to add to the config file for the given property */
	static FString GenerateExportedPropertyLine(const FString& PropertyName, const FString& PropertyValue);

	/** Append a correctly escaped line to add to the config file for the given property */
	static void AppendExportedPropertyLine(FString& Out, const FString& PropertyName, const FString& PropertyValue);

	/** Checks the command line for any overridden config settings */
	CORE_API static void OverrideFromCommandline(FConfigFile* File, const FString& Filename);

	/** Checks the command line for any overridden config file settings */
	CORE_API static bool OverrideFileFromCommandline(FString& Filename);

	/** Appends a new INI file to the SourceIniHierarchy and combines it with the current contents */
	CORE_API void AddDynamicLayerToHierarchy(const FString& Filename);

	UE_DEPRECATED(5.0, "Call AddDynamicLayerToHierarchy. You also may need to call GetConfigFilename to get the right FConfigFile")
	CORE_API void AddDynamicLayerToHeirarchy(const FString& Filename) { AddDynamicLayerToHierarchy(Filename); }

	friend FArchive& operator<<(FArchive& Ar, FConfigFile& ConfigFile);
private:

	// This holds per-object config class names, with their ArrayOfStructKeys. Since the POC sections are all unique,
	// we can't track it just in that section. This is expected to be empty/small
	TMap<FString, TMap<FName, FString> > PerObjectConfigArrayOfStructKeys;

	/** 
	 * Save the source hierarchy which was loaded out to a backup file so we can check future changes in the base/default configs
	 */
	void SaveSourceToBackupFile();

	/** 
	 * Process the property for Writing to a default file. We will need to check for array operations, as default ini's rely on this
	 * being correct to function properly
	 *
	 * @param IniCombineThreshold - Cutoff level for combining ini (to prevent applying changes from the same ini that we're writing)
	 * @param InCompletePropertyToProcess - The complete property which we need to process for saving.
	 * @param OutText - The stream we are processing the array to
	 * @param SectionName - The section name the array property is being written to
	 * @param PropertyName - The property name of the array
	 */
	void ProcessPropertyAndWriteForDefaults(int32 IniCombineThreshold, const TArray<const FConfigValue*>& InCompletePropertyToProcess, FString& OutText, const FString& SectionName, const FString& PropertyName);

	/**
	 * Creates a chain of ini filenames to load and combine.
	 *
	 * @param InBaseIniName Ini name.
	 * @param InPlatformName Platform name, nullptr means to use the current platform
	 * @param OutHierarchy An array which is to receive the generated hierachy of ini filenames.
	 */
	void AddStaticLayersToHierarchy(const TCHAR* InBaseIniName, const TCHAR* InPlatformName, const TCHAR* EngineConfigDir, const TCHAR* SourceConfigDir);

	// for AddStaticLayersToHierarchy
	friend class FConfigCacheIni;
};

FArchive& operator<<(FArchive& Ar, FConfigFile& ConfigFile);

/**
 * Declares a delegate type that's used by the config system to allow iteration of key value pairs.
 */
DECLARE_DELEGATE_TwoParams(FKeyValueSink, const TCHAR*, const TCHAR*);

enum class EConfigCacheType : uint8
{
	// this type of config cache will write its files to disk during Flush (i.e. GConfig)
	DiskBacked,
	// this type of config cache is temporary and will never write to disk (only load from disk)
	Temporary,
};

// Set of all cached config files.
class CORE_API FConfigCacheIni : private TMap<FString,FConfigFile>
{
public:

	static void SetIniCacheSet(const TSet<FString>* InIniCacheSet)
	{
		IniCacheSet = InIniCacheSet;
	}

	static const TSet<FString>* GetIniCacheSet()
	{
		return IniCacheSet;
	}

private:

	static const TSet<FString>* IniCacheSet;
	
public:
	
	// Basic functions.
	FConfigCacheIni(EConfigCacheType Type);

	/** DO NOT USE. This constructor is for internal usage only for hot-reload purposes. */
	FConfigCacheIni();

	virtual ~FConfigCacheIni();

	/**
	* Disables any file IO by the config cache system
	*/
	virtual void DisableFileOperations();

	/**
	* Re-enables file IO by the config cache system
	*/
	virtual void EnableFileOperations();

	/**
	 * Returns whether or not file operations are disabled
	 */
	virtual bool AreFileOperationsDisabled();

	/**
	 * @return true after after the basic .ini files have been loaded
	 */
	bool IsReadyForUse()
	{
		return bIsReadyForUse;
	}

	/**
	* Prases apart an ini section that contains a list of 1-to-N mappings of strings in the following format
	*	 [PerMapPackages]
	*	 MapName=Map1
	*	 Package=PackageA
	*	 Package=PackageB
	*	 MapName=Map2
	*	 Package=PackageC
	*	 Package=PackageD
	* 
	* @param Section Name of section to look in
	* @param KeyOne Key to use for the 1 in the 1-to-N (MapName in the above example)
	* @param KeyN Key to use for the N in the 1-to-N (Package in the above example)
	* @param OutMap Map containing parsed results
	* @param Filename Filename to use to find the section
	*
	* NOTE: The function naming is weird because you can't apparently have an overridden function differnt only by template type params
	*/
	virtual void Parse1ToNSectionOfStrings(const TCHAR* Section, const TCHAR* KeyOne, const TCHAR* KeyN, TMap<FString, TArray<FString> >& OutMap, const FString& Filename);

	/**
	* Parses apart an ini section that contains a list of 1-to-N mappings of names in the following format
	*	 [PerMapPackages]
	*	 MapName=Map1
	*	 Package=PackageA
	*	 Package=PackageB
	*	 MapName=Map2
	*	 Package=PackageC
	*	 Package=PackageD
	* 
	* @param Section Name of section to look in
	* @param KeyOne Key to use for the 1 in the 1-to-N (MapName in the above example)
	* @param KeyN Key to use for the N in the 1-to-N (Package in the above example)
	* @param OutMap Map containing parsed results
	* @param Filename Filename to use to find the section
	*
	* NOTE: The function naming is weird because you can't apparently have an overridden function differnt only by template type params
	*/
	virtual void Parse1ToNSectionOfNames(const TCHAR* Section, const TCHAR* KeyOne, const TCHAR* KeyN, TMap<FName, TArray<FName> >& OutMap, const FString& Filename);

	/**
	 * Finds the in-memory config file for a config cache filename.
	 *
	 * @param A known key like GEngineIni, or the return value of GetConfigFilename
	 *
	 * @return The existing config file or null if it does not exist in memory
	 */
	FConfigFile* FindConfigFile(const FString& Filename);

	/**
	 * Finds, loads, or creates the in-memory config file for a config cache filename.
	 * 
	 * @param A known key like GEngineIni, or the return value of GetConfigFilename
	 * 
	 * @return A new or existing config file
	 */
	FConfigFile* Find(const FString& InFilename);

	/**
	 * Reports whether an FConfigFile* is pointing to a config file inside of this
	 * Used for downstream functions to check whether a config file they were passed came from this ConfigCacheIni or from 
	 * a different source such as LoadLocalIniFile
	 */
	bool ContainsConfigFile(const FConfigFile* ConfigFile) const;

	UE_DEPRECATED(5.0, "CreateIfNotFound is deprecated, please use the overload without this parameter or FindConfigFile")
	FConfigFile* Find(const FString& Filename, bool CreateIfNotFound);

	/** Finds Config file that matches the base name such as "Engine" */
	FConfigFile* FindConfigFileWithBaseName(FName BaseName);

	using Super = TMap<FString, FConfigFile>;
	FConfigFile& Add(const FString& Filename, const FConfigFile& File)
	{
		return Super::Add(Filename, File);
	}
	int32 Remove(const FString& Filename)
	{
		return Super::Remove(Filename);
	}
	TArray<FString> GetFilenames();


	void Flush( bool Read, const FString& Filename=TEXT("") );

	void LoadFile( const FString& InFilename, const FConfigFile* Fallback = NULL, const TCHAR* PlatformString = NULL );
	void SetFile( const FString& InFilename, const FConfigFile* NewConfigFile );
	void UnloadFile( const FString& Filename );
	void Detach( const FString& Filename );

	bool GetString( const TCHAR* Section, const TCHAR* Key, FString& Value, const FString& Filename );
	bool GetText( const TCHAR* Section, const TCHAR* Key, FText& Value, const FString& Filename );
	bool GetSection( const TCHAR* Section, TArray<FString>& Result, const FString& Filename );
	bool DoesSectionExist(const TCHAR* Section, const FString& Filename);
	/**
	 * @param Force Whether to create the Section on Filename if it did not exist previously.
	 * @param Const If Const (and not Force), then it will not modify File->Dirty. If not Const (or Force is true), then File->Dirty will be set to true.
	 */
	FConfigSection* GetSectionPrivate( const TCHAR* Section, const bool Force, const bool Const, const FString& Filename );
	void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const FString& Filename );
	void SetText( const TCHAR* Section, const TCHAR* Key, const FText& Value, const FString& Filename );
	bool RemoveKey( const TCHAR* Section, const TCHAR* Key, const FString& Filename );
	bool EmptySection( const TCHAR* Section, const FString& Filename );
	bool EmptySectionsMatchingString( const TCHAR* SectionString, const FString& Filename );

	/**
	 * For a base ini name, gets the config cache filename key that is used by other functions like Find.
	 * This will be the base name for known configs like Engine and the destination filename for others.
	 *
	 * @param IniBaseName Base name of the .ini (Engine, Game, CustomSystem)
	 *
	 * @return Filename key used by other cache functions
	 */
	FString GetConfigFilename(const TCHAR* BaseIniName);

	/**
	 * Retrieve a list of all of the config files stored in the cache
	 *
	 * @param ConfigFilenames Out array to receive the list of filenames
	 */
	void GetConfigFilenames(TArray<FString>& ConfigFilenames);

	/**
	 * Retrieve the names for all sections contained in the file specified by Filename
	 *
	 * @param	Filename			the file to retrieve section names from
	 * @param	out_SectionNames	will receive the list of section names
	 *
	 * @return	true if the file specified was successfully found;
	 */
	bool GetSectionNames( const FString& Filename, TArray<FString>& out_SectionNames );

	/**
	 * Retrieve the names of sections which contain data for the specified PerObjectConfig class.
	 *
	 * @param	Filename			the file to retrieve section names from
	 * @param	SearchClass			the name of the PerObjectConfig class to retrieve sections for.
	 * @param	out_SectionNames	will receive the list of section names that correspond to PerObjectConfig sections of the specified class
	 * @param	MaxResults			the maximum number of section names to retrieve
	 *
	 * @return	true if the file specified was found and it contained at least 1 section for the specified class
	 */
	bool GetPerObjectConfigSections( const FString& Filename, const FString& SearchClass, TArray<FString>& out_SectionNames, int32 MaxResults=1024 );

	void Exit();

	/**
	 * Prints out the entire config set, or just a single file if an ini is specified
	 * 
	 * @param Ar the device to write to
	 * @param IniName An optional ini name to restrict the writing to (Engine or WrangleContent) - meant to be used with "final" .ini files (not Default*)
	 */
	void Dump(FOutputDevice& Ar, const TCHAR* IniName=NULL);

	/**
	 * Dumps memory stats for each file in the config cache to the specified archive.
	 *
	 * @param	Ar	the output device to dump the results to
	 */
	virtual void ShowMemoryUsage( FOutputDevice& Ar );

	/**
	 * USed to get the max memory usage for the FConfigCacheIni
	 *
	 * @return the amount of memory in byes
	 */
	virtual SIZE_T GetMaxMemoryUsage();

	/**
	 * allows to iterate through all key value pairs
	 * @return false:error e.g. Section or Filename not found
	 */
	virtual bool ForEachEntry(const FKeyValueSink& Visitor, const TCHAR* Section, const FString& Filename);

	// Derived functions.
	FString GetStr
	(
		const TCHAR*		Section, 
		const TCHAR*		Key, 
		const FString&	Filename 
	);
	bool GetInt
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		int32&				Value,
		const FString&	Filename
	);
	bool GetFloat
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		float&				Value,
		const FString&	Filename
	);
	bool GetDouble
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		double&				Value,
		const FString&	Filename
	);
	bool GetBool
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		bool&				Value,
		const FString&	Filename
	);
	int32 GetArray
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		TArray<FString>&	out_Arr,
		const FString&	Filename
	);
	/** Loads a "delimited" list of strings
	 * @param Section - Section of the ini file to load from
	 * @param Key - The key in the section of the ini file to load
	 * @param out_Arr - Array to load into
	 * @param Filename - Ini file to load from
	 */
	int32 GetSingleLineArray
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		TArray<FString>&	out_Arr,
		const FString&	Filename
	);
	bool GetColor
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FColor&				Value,
		const FString&	Filename
	);
	bool GetVector2D(
		const TCHAR*   Section,
		const TCHAR*   Key,
		FVector2D&     Value,
		const FString& Filename);
	bool GetVector
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FVector&			Value,
		const FString&	Filename
	);
	bool GetVector4
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FVector4&			Value,
		const FString&	Filename
	);
	bool GetRotator
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FRotator&			Value,
		const FString&	Filename
	);

	/* Generic versions for use with templates */
	bool GetValue(const TCHAR* Section, const TCHAR* Key, FString& Value, const FString& Filename)
	{
		return GetString(Section, Key, Value, Filename);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, FText& Value, const FString& Filename)
	{
		return GetText(Section, Key, Value, Filename);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, int32& Value, const FString& Filename)
	{
		return GetInt(Section, Key, Value, Filename);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, float& Value, const FString& Filename)
	{
		return GetFloat(Section, Key, Value, Filename);
	}
	bool GetValue(const TCHAR* Section, const TCHAR* Key, bool& Value, const FString& Filename)
	{
		return GetBool(Section, Key, Value, Filename);
	}
	int32 GetValue(const TCHAR* Section, const TCHAR* Key, TArray<FString>& Value, const FString& Filename)
	{
		return GetArray(Section, Key, Value, Filename);
	}

	void SetInt
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		int32					Value,
		const FString&	Filename
	);
	void SetFloat
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		float				Value,
		const FString&	Filename
	);
	void SetDouble
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		double				Value,
		const FString&	Filename
	);
	void SetBool
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		bool				Value,
		const FString&	Filename
	);
	void SetArray
	(
		const TCHAR*			Section,
		const TCHAR*			Key,
		const TArray<FString>&	Value,
		const FString&		Filename
	);
	/** Saves a "delimited" list of strings
	 * @param Section - Section of the ini file to save to
	 * @param Key - The key in the section of the ini file to save
	 * @param out_Arr - Array to save from
	 * @param Filename - Ini file to save to
	 */
	void SetSingleLineArray
	(
		const TCHAR*			Section,
		const TCHAR*			Key,
		const TArray<FString>&	In_Arr,
		const FString&		Filename
	);
	void SetColor
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FColor				Value,
		const FString&	Filename
	);
	void SetVector2D(
		const TCHAR*   Section,
		const TCHAR*   Key,
		FVector2D      Value,
		const FString& Filename);
	void SetVector
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FVector				Value,
		const FString&	Filename
	);
	void SetVector4
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		const FVector4&		Value,
		const FString&	Filename
	);
	void SetRotator
	(
		const TCHAR*		Section,
		const TCHAR*		Key,
		FRotator			Value,
		const FString&	Filename
	);

	// Static helper functions

	/**
	 * Creates GConfig, loads the standard global ini files (Engine, Editor, etc),
	 * fills out GEngineIni, etc. and marks GConfig as ready for use
	 */
	static void InitializeConfigSystem();

	/**
	 * Returns the Custom Config string, which if set will load additional config files from Config/Custom/{CustomConfig}/DefaultX.ini to allow different types of builds.
	 * It can be set from a game Target.cs file with CustomConfig = "Name".
	 * Or in development, it can be overridden with a -CustomConfig=Name command line parameter.
	 */
	static const FString& GetCustomConfigString();

	/**
	 * Calculates the name of a dest (generated) .ini file for a given base (ie Engine, Game, etc)
	 *
	 * @param IniBaseName Base name of the .ini (Engine, Game)
	 * @param PlatformName Name of the platform to get the .ini path for (nullptr means to use the current platform)
	 * @param GeneratedConfigDir The base folder that will contain the generated config files.
	 *
	 * @return Standardized .ini filename
	 */
	static FString GetDestIniFilename(const TCHAR* BaseIniName, const TCHAR* PlatformName, const TCHAR* GeneratedConfigDir);

	/**
	 * Loads and generates a destination ini file and adds it to GConfig:
	 *   - Looking on commandline for override source/dest .ini filenames
	 *   - Generating the name for the engine to refer to the ini
	 *   - Loading a source .ini file hierarchy
	 *   - Filling out an FConfigFile
	 *   - Save the generated ini
	 *   - Adds the FConfigFile to GConfig
	 *
	 * @param FinalIniFilename The output name of the generated .ini file (in Game\Saved\Config)
	 * @param BaseIniName The "base" ini name, with no extension (ie, Engine, Game, etc)
	 * @param Platform The platform to load the .ini for (if NULL, uses current)
	 * @param bForceReload If true, the destination .in will be regenerated from the source, otherwise this will only process if the dest isn't in GConfig
	 * @param bRequireDefaultIni If true, the Default*.ini file is required to exist when generating the final ini file.
	 * @param bAllowGeneratedIniWhenCooked If true, the engine will attempt to load the generated/user INI file when loading cooked games
	 * @param GeneratedConfigDir The location where generated config files are made.
	 * @return true if the final ini was created successfully.
	 */
	static bool LoadGlobalIniFile(FString& FinalIniFilename, const TCHAR* BaseIniName, const TCHAR* Platform = NULL, bool bForceReload = false, bool bRequireDefaultIni = false, bool bAllowGeneratedIniWhenCooked = true, bool bAllowRemoteConfig = true, const TCHAR* GeneratedConfigDir = *FPaths::GeneratedConfigDir(), FConfigCacheIni* ConfigSystem=GConfig);

	/**
	 * Load an ini file directly into an FConfigFile, and nothing is written to GConfig or disk. 
	 * The passed in .ini name can be a "base" (Engine, Game) which will be modified by platform and/or commandline override,
	 * or it can be a full ini filename (ie WrangleContent) loaded from the Source config directory
	 *
	 * @param ConfigFile The output object to fill
	 * @param IniName Either a Base ini name (Engine) or a full ini name (WrangleContent). NO PATH OR EXTENSION SHOULD BE USED!
	 * @param bIsBaseIniName true if IniName is a Base name, which can be overridden on commandline, etc.
	 * @param Platform The platform to use for Base ini names, NULL means to use the current platform
	 * @param bForceReload force reload the ini file from disk this is required if you make changes to the ini file not using the config system as the hierarchy cache will not be updated in this case
	 * @return true if the ini file was loaded successfully
	 */
	static bool LoadLocalIniFile(FConfigFile& ConfigFile, const TCHAR* IniName, bool bIsBaseIniName, const TCHAR* Platform=NULL, bool bForceReload=false);

	/**
	 * Load an ini file directly into an FConfigFile from the specified config folders, optionally writing to disk. 
	 * The passed in .ini name can be a "base" (Engine, Game) which will be modified by platform and/or commandline override,
	 * or it can be a full ini filename (ie WrangleContent) loaded from the Source config directory
	 *
	 * @param ConfigFile The output object to fill
	 * @param IniName Either a Base ini name (Engine) or a full ini name (WrangleContent). NO PATH OR EXTENSION SHOULD BE USED!
	 * @param EngineConfigDir Engine config directory.
	 * @param SourceConfigDir Game config directory.
	 * @param bIsBaseIniName true if IniName is a Base name, which can be overridden on commandline, etc.
	 * @param Platform The platform to use for Base ini names
	 * @param bForceReload force reload the ini file from disk this is required if you make changes to the ini file not using the config system as the hierarchy cache will not be updated in this case
	 * @param bWriteDestIni write out a destination ini file to the Saved folder, only valid if bIsBaseIniName is true
	 * @param bAllowGeneratedIniWhenCooked If true, the engine will attempt to load the generated/user INI file when loading cooked games
	 * @param GeneratedConfigDir The location where generated config files are made.
	 * @return true if the ini file was loaded successfully
	 */
	static bool LoadExternalIniFile(FConfigFile& ConfigFile, const TCHAR* IniName, const TCHAR* EngineConfigDir, const TCHAR* SourceConfigDir, bool bIsBaseIniName, const TCHAR* Platform=NULL, bool bForceReload=false, bool bWriteDestIni=false, bool bAllowGeneratedIniWhenCooked = true, const TCHAR* GeneratedConfigDir = *FPaths::GeneratedConfigDir());

	/**
	 * Needs to be called after GConfig is set and LoadCoalescedFile was called.
	 * Loads the state of console variables.
	 * Works even if the variable is registered after the ini file was loaded.
	 */
	static void LoadConsoleVariablesFromINI();

	/**
	 * Save the current config cache state into a file for bootstrapping other processes.
	 */
	void SaveCurrentStateForBootstrap(const TCHAR* Filename);

	void Serialize(FArchive& Ar);


	struct FKnownConfigFiles
	{
		FKnownConfigFiles();

		// Write out this for binary config serialization
		friend FArchive& operator<<(FArchive& Ar, FKnownConfigFiles& Names);

		// setup GEngineIni based on this structure's values
		void SetGlobalIniStringsFromMembers();

		// given an name ("Engine") return the FConfigFile for it
		const FConfigFile* GetFile(FName Name);
		// given an name ("Engine") return the modifiable FConfigFile for it
		FConfigFile* GetMutableFile(FName Name);

		// get the disk-based filename for the given known ini name
		const FString& GetFilename(FName Name);


		// create the list of members for the known inis (Engine, Game, etc) See the top of this file for the list
		struct FKnownConfigFile
		{
			FName IniName;
			FString IniPath;
			FConfigFile IniFile;
		};

		// array of all known filesd
		FKnownConfigFile Files[(uint8)EKnownIniFile::NumKnownFiles];
	};

	/**
	 * Load the standard (used on all platforms) ini files, like Engine, Input, etc
	 *
	 * @param PlatformName Ini name of the platform this is loadenig for (or nullptr for current platform)
	 * @param bDefaultEngineIniRequired True if the engine ini is required
	 *
	 * return True if the engine ini was loaded
	 */
	bool InitializeKnownConfigFiles(const TCHAR* PlatformName, bool bDefaultEngineIniRequired);

	/**
	 * Returns true if the given name is one of the known configs, where the matching G****Ini property is going to match the 
	 * base name ("Engine" returns true, which means GEngineIni's value is just "Engine")
	 */
	bool IsKnownConfigName(FName ConfigName);

	/**
	 * Create GConfig from a saved file
	 */
	static bool CreateGConfigFromSaved(const TCHAR* Filename);

	/**
	 * Retrieve the fully processed ini system for another platform. The editor will start loading these 
	 * in the background on startup
	 */
	static FConfigCacheIni* ForPlatform(FName PlatformName);

	/**
	 * Wipe all cached platform configs. Next ForPlatform call will load on-demand the platform configs
	 */
	static void ClearOtherPlatformConfigs();

private:
#if WITH_EDITOR
	/** We only auto-initialize other platform configs in the editor to not slow down programs like ShaderCOmpileWorker */
	static void AsyncInitializeConfigForPlatforms();
#endif

	/** Serialize a bootstrapping state into or from an archive */
	void SerializeStateForBootstrap_Impl(FArchive& Ar);

	/** true if file operations should not be performed */
	bool bAreFileOperationsDisabled;

	/** true after the base .ini files have been loaded, and GConfig is generally "ready for use" */
	bool bIsReadyForUse;
	
	/** The type of the cache (basically, do we call Flush in the destructor) */
	EConfigCacheType Type;

	/** The filenames for the known files in this config */
	FKnownConfigFiles KnownFiles;
};

UE_DEPRECATED(4.24, "This functionality to generate Scalability@Level section string has been moved to Scalability.cpp. Explictly construct section you need manually.")
CORE_API void ApplyCVarSettingsGroupFromIni(const TCHAR* InSectionBaseName, int32 InGroupNumber, const TCHAR* InIniFilename, uint32 SetBy);

UE_DEPRECATED(4.24, "This functionality to generate Scalability@Level section string has been moved to Scalability.cpp. Explictly construct section you need manually.")
CORE_API void ApplyCVarSettingsGroupFromIni(const TCHAR* InSectionBaseName, const TCHAR* InSectionTag, const TCHAR* InIniFilename, uint32 SetBy);

/**
 * Helper function to read the contents of an ini file and a specified group of cvar parameters, where sections in the ini file are marked [InName]
 * @param InSectionBaseName - The base name of the section to apply cvars from
 * @param InIniFilename - The ini filename
 * @param SetBy anything in ECVF_LastSetMask e.g. ECVF_SetByScalability
 */
CORE_API void ApplyCVarSettingsFromIni(const TCHAR* InSectionBaseName, const TCHAR* InIniFilename, uint32 SetBy, bool bAllowCheating = false);

/**
 * Helper function to operate a user defined function for each CVar key/value pair in the specified section in an ini file
 * @param InSectionName - The name of the section to apply cvars from
 * @param InIniFilename - The ini filename
 * @param InEvaluationFunction - The evaluation function to be called for each key/value pair
 */
CORE_API void ForEachCVarInSectionFromIni(const TCHAR* InSectionName, const TCHAR* InIniFilename, TFunction<void(IConsoleVariable* CVar, const FString& KeyString, const FString& ValueString)> InEvaluationFunction);

/**
 * CVAR Ini history records all calls to ApplyCVarSettingsFromIni and can re run them 
 */

/**
 * Helper function to start recording ApplyCVarSettings function calls 
 * uses these to generate a history of applied ini settings sections
 */
CORE_API void RecordApplyCVarSettingsFromIni();

/**
 * Helper function to reapply inis which have been applied after RecordCVarIniHistory was called
 */
CORE_API void ReapplyRecordedCVarSettingsFromIni();

/**
 * Helper function to clean up ini history
 */
CORE_API void DeleteRecordedCVarSettingsFromIni();

/**
 * Helper function to start recording config reads
 */
CORE_API void RecordConfigReadsFromIni();

/**
 * Helper function to dump config reads to csv after RecordConfigReadsFromIni was called
 */
CORE_API void DumpRecordedConfigReadsFromIni();

/**
 * Helper function to clean up config read history
 */
CORE_API void DeleteRecordedConfigReadsFromIni();

/**
 * Helper function to deal with "True","False","Yes","No","On","Off"
 */
CORE_API const TCHAR* ConvertValueFromHumanFriendlyValue(const TCHAR* Value);