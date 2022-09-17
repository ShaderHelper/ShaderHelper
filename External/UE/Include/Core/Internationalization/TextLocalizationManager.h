// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Misc/Crc.h"
#include "Containers/UnrealString.h"
#include "Containers/Set.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Delegates/Delegate.h"
#include "Internationalization/TextKey.h"
#include "Internationalization/LocTesting.h"
#include "Internationalization/LocKeyFuncs.h"
#include "Internationalization/LocalizedTextSourceTypes.h"

struct FPolyglotTextData;
class ILocalizedTextSource;
class IPakFile;
class FTextLocalizationResource;

enum class ETextLocalizationManagerInitializedFlags : uint8
{
	None = 0,
	Engine = 1<<0,
	Game = 1<<1,
	Initializing = 1<<2
};
ENUM_CLASS_FLAGS(ETextLocalizationManagerInitializedFlags);

/** Singleton class that manages display strings for FText. */
class CORE_API FTextLocalizationManager
{
	friend CORE_API void BeginPreInitTextLocalization();
	friend CORE_API void BeginInitTextLocalization();
	friend CORE_API void InitEngineTextLocalization();
	friend CORE_API void BeginInitGameTextLocalization();
	friend CORE_API void EndInitGameTextLocalization();
	friend CORE_API void InitGameTextLocalization();

private:

	/** Data struct for tracking a display string. */
	struct FDisplayStringEntry
	{
		FTextConstDisplayStringRef DisplayString;
#if WITH_EDITORONLY_DATA
		FTextKey LocResID;
#endif
#if ENABLE_LOC_TESTING
		FTextConstDisplayStringPtr NativeStringBackup;
#endif
		uint32 SourceStringHash;

		FDisplayStringEntry(const FTextKey& InLocResID, const uint32 InSourceStringHash, const FTextConstDisplayStringRef& InDisplayString)
			: DisplayString(InDisplayString)
#if WITH_EDITORONLY_DATA
			, LocResID(InLocResID)
#endif
			, SourceStringHash(InSourceStringHash)
		{
		}

		/** 
		* Returns true if the display string entry contains invalid display string data. 
		*/
		bool IsEmpty() const
		{
			return SourceStringHash == 0 && DisplayString->IsEmpty();
		}
	};

	/** Manages the currently loaded or registered text localizations. */
	typedef TMap<FTextId, FDisplayStringEntry> FDisplayStringLookupTable;

private:
	ETextLocalizationManagerInitializedFlags InitializedFlags = ETextLocalizationManagerInitializedFlags::None;
	
	bool IsInitialized() const
	{
		return InitializedFlags != ETextLocalizationManagerInitializedFlags::None;
	}

	bool IsInitializing() const
	{
		return EnumHasAnyFlags(InitializedFlags, ETextLocalizationManagerInitializedFlags::Initializing);
	}

	mutable FCriticalSection DisplayStringLookupTableCS;
	FDisplayStringLookupTable DisplayStringLookupTable;

	mutable FRWLock TextRevisionRW;
	TMap<FTextId, uint16> LocalTextRevisions;
	uint16 TextRevisionCounter;

#if WITH_EDITOR
	uint8 GameLocalizationPreviewAutoEnableCount;
	bool bIsGameLocalizationPreviewEnabled;
	bool bIsLocalizationLocked;
#endif

	FTextLocalizationManager();
	friend class FLazySingleton;
	
public:

	/** Singleton accessor */
	static FTextLocalizationManager& Get();
	static void TearDown();

	void DumpMemoryInfo() const;
	void CompactDataStructures();

	/**
	 * Get the language that will be requested during localization initialization, based on the hierarchy of: command line -> configs -> OS default.
	 */
	FString GetRequestedLanguageName() const;

	/**
	 * Get the locale that will be requested during localization initialization, based on the hierarchy of: command line -> configs -> OS default.
	 */
	FString GetRequestedLocaleName() const;

	/**
	 * Given a localization category, get the native culture for the category (if known).
	 * @return The native culture for the given localization category, or an empty string if the native culture is unknown.
	 */
	FString GetNativeCultureName(const ELocalizedTextSourceCategory InCategory) const;

	/**
	 * Get a list of culture names that we have localized resource data for (ELocalizationLoadFlags controls which resources should be checked).
	 */
	TArray<FString> GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags) const;

	/**
	 * Register a localized text source with the text localization manager.
	 */
	void RegisterTextSource(const TSharedRef<ILocalizedTextSource>& InLocalizedTextSource, const bool InRefreshResources = true);

	/**
	 * Register a polyglot text data with the text localization manager.
	 */
	void RegisterPolyglotTextData(const FPolyglotTextData& InPolyglotTextData, const bool InAddDisplayString = true);
	void RegisterPolyglotTextData(TArrayView<const FPolyglotTextData> InPolyglotTextDataArray, const bool InAddDisplayStrings = true);

	/**	Finds and returns the display string with the given namespace and key, if it exists.
	 *	Additionally, if a source string is specified and the found localized display string was not localized from that source string, null will be returned. */
	FTextConstDisplayStringPtr FindDisplayString(const FTextKey& Namespace, const FTextKey& Key, const FString* const SourceString = nullptr) const;

	/**	Returns a display string with the given namespace and key.
	 *	If no display string exists, it will be created using the source string or an empty string if no source string is provided.
	 *	If a display string exists ...
	 *		... but it was not localized from the specified source string, the display string will be set to the specified source and returned.
	 *		... and it was localized from the specified source string (or none was provided), the display string will be returned.
	*/
	FTextConstDisplayStringRef GetDisplayString(const FTextKey& Namespace, const FTextKey& Key, const FString* const SourceString);

#if WITH_EDITORONLY_DATA
	/** If an entry exists for the specified namespace and key, returns true and provides the localization resource identifier from which it was loaded. Otherwise, returns false. */
	bool GetLocResID(const FTextKey& Namespace, const FTextKey& Key, FString& OutLocResId) const;
#endif

	UE_DEPRECATED(5.0, "FindNamespaceAndKeyFromDisplayString no longer functions! Use FTextInspector::GetTextId instead.")
	bool FindNamespaceAndKeyFromDisplayString(const FTextConstDisplayStringPtr& InDisplayString, FString& OutNamespace, FString& OutKey) const { return false; }

	UE_DEPRECATED(5.0, "FindNamespaceAndKeyFromDisplayString no longer functions! Use FTextInspector::GetTextId instead.")
	bool FindNamespaceAndKeyFromDisplayString(const FTextConstDisplayStringPtr& InDisplayString, FTextKey& OutNamespace, FTextKey& OutKey) const { return false; }
	
	/**	Attempts to register the specified display string, associating it with the specified namespace and key.
	 *	Returns true if the display string has been or was already associated with the namespace and key.
	 *	Returns false if the display string was already associated with another namespace and key or the namespace and key are already in use by another display string.
	 */
	bool AddDisplayString(const FTextDisplayStringRef& DisplayString, const FTextKey& Namespace, const FTextKey& Key);

	/** Updates display string entries and adds new display string entries based on localizations found in a specified localization resource. */
	void UpdateFromLocalizationResource(const FString& LocalizationResourceFilePath);
	void UpdateFromLocalizationResource(const FTextLocalizationResource& TextLocalizationResource);

	/** Reloads resources for the current culture. */
	void RefreshResources();

	/**
	 * Returns the current text revision number. This value can be cached when caching information from the text localization manager.
	 * If the revision does not match, cached information may be invalid and should be recached.
	 */
	uint16 GetTextRevision() const;

	/**
	 * Attempts to find a local revision history for the given text ID.
	 * This will only be set if the display string has been changed since the localization manager version has been changed (eg, if it has been edited while keeping the same key).
	 * @return The local revision, or 0 if there have been no changes since a global history change.
	 */
	uint16 GetLocalRevisionForTextId(const FTextId& InTextId) const;

	/**
	 * Get both the global and local revision for the given text ID.
	 * @see GetTextRevision and GetLocalRevisionForTextId.
	 */
	void GetTextRevisions(const FTextId& InTextId, uint16& OutGlobalTextRevision, uint16& OutLocalTextRevision) const;

#if WITH_EDITOR
	/**
	 * Enable the game localization preview using the current "preview language" setting, or the native culture if no "preview language" is set.
	 * @note This is the same as calling EnableGameLocalizationPreview with the current "preview language" setting.
	 */
	void EnableGameLocalizationPreview();

	/**
	 * Enable the game localization preview using the given language, or the native language if the culture name is empty.
	 * @note This will also lockdown localization editing if the given language is a non-native game language (to avoid accidentally baking out translations as source data in assets).
	 */
	void EnableGameLocalizationPreview(const FString& CultureName);

	/**
	 * Disable the game localization preview.
	 * @note This is the same as calling EnableGameLocalizationPreview with the native game language (or an empty string).
	 */
	void DisableGameLocalizationPreview();

	/**
	 * Is the game localization preview enabled for a non-native language?
	 */
	bool IsGameLocalizationPreviewEnabled() const;

	/**
	 * Notify that the game localization preview should automatically enable itself under certain circumstances 
	 * (such as changing the preview language via the UI) due to a state change (such as PIE starting).
	 * @note This must be paired with a call to PopAutoEnableGameLocalizationPreview.
	 */
	void PushAutoEnableGameLocalizationPreview();

	/**
	 * Notify that the game localization preview should no longer automatically enable itself under certain circumstances 
	 * (such as changing the preview language via the UI) due to a state change (such as PIE ending).
	 * @note This must be paired with a call to PushAutoEnableGameLocalizationPreview.
	 */
	void PopAutoEnableGameLocalizationPreview();

	/**
	 * Should the game localization preview automatically enable itself under certain circumstances?
	 */
	bool ShouldGameLocalizationPreviewAutoEnable() const;

	/**
	 * Configure the "preview language" setting used for the game localization preview.
	 */
	void ConfigureGameLocalizationPreviewLanguage(const FString& CultureName);

	/**
	 * Get the configured "preview language" setting used for the game localization preview (if any).
	 */
	FString GetConfiguredGameLocalizationPreviewLanguage() const;

	/**
	 * Is the localization of this game currently locked? (ie, can it be edited in the UI?).
	 */
	bool IsLocalizationLocked() const;
#endif

	/** Event type for immediately reacting to changes in display strings for text. */
	DECLARE_EVENT(FTextLocalizationManager, FTextRevisionChangedEvent)
	FTextRevisionChangedEvent OnTextRevisionChangedEvent;

private:
	/** Callback for when a PAK file is loaded. Loads any chunk specific localization resources. */
	void OnPakFileMounted(const IPakFile& PakFile);

	/** Callback for changes in culture. Loads the new culture's localization resources. */
	void OnCultureChanged();

	/** Loads localization resources for the specified culture, optionally loading localization resources that are editor-specific or game-specific. */
	void LoadLocalizationResourcesForCulture(const FString& CultureName, const ELocalizationLoadFlags LocLoadFlags);

	/** Loads localization resources for the specified prioritized cultures, optionally loading localization resources that are editor-specific or game-specific. */
	void LoadLocalizationResourcesForPrioritizedCultures(TArrayView<const FString> PrioritizedCultureNames, const ELocalizationLoadFlags LocLoadFlags);

	/** Updates display string entries and adds new display string entries based on provided native text. */
	void UpdateFromNative(FTextLocalizationResource&& TextLocalizationResource, const bool bDirtyTextRevision = true);

	/** Updates display string entries and adds new display string entries based on provided localizations. */
	void UpdateFromLocalizations(FTextLocalizationResource&& TextLocalizationResource, const bool bDirtyTextRevision = true);

	/** Dirties the local revision counter for the given text ID by incrementing it (or adding it) */
	void DirtyLocalRevisionForTextId(const FTextId& InTextId);

	/** Dirties the text revision counter by incrementing it, causing a revision mismatch for any information cached before this happens.  */
	void DirtyTextRevision();

	/** Array of registered localized text sources, sorted by priority (@see RegisterTextSource) */
	TArray<TSharedPtr<ILocalizedTextSource>> LocalizedTextSources;

	/** The LocRes text source (this is also added to LocalizedTextSources, but we keep a pointer to it directly so we can patch in chunked LocRes data at runtime) */
	TSharedPtr<class FLocalizationResourceTextSource> LocResTextSource;

	/** The polyglot text source (this is also added to LocalizedTextSources, but we keep a pointer to it directly so we can add new polyglot data to it at runtime) */
	TSharedPtr<class FPolyglotTextSource> PolyglotTextSource;
};
