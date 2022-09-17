// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "HAL/CriticalSection.h"

namespace UE::Core::Private
{
	const TCHAR*   GetBindingType(const TCHAR* Ptr);
	const FString& GetBindingType(const FString& Str);

	// This is used to force the arguments of FString::Combine to implicitly convert (if necessary)
	// to FString when calling CombineImpl(), allowing them to remain as temporaries on the stack
	// so that they stay allocated during the combining process.
	//
	// Pointer arguments are passed without causing FString temporaries to be created,
	// and FString arguments are referenced directly without creating extra copies.
	template <typename T>
	using TToStringType_T = decltype(GetBindingType(std::declval<T>()));
}

/**
 * Path helpers for retrieving game dir, engine dir, etc.
 */
class CORE_API FPaths
{
public:

	/**
	 * Should the "saved" directory structures be rooted in the user dir or relative to the "engine/game" 
	 */
	static bool ShouldSaveToUserDir();

	/**
	 * Returns the directory the application was launched from (useful for commandline utilities)
	 */
	static FString LaunchDir();
	 
	/** 
	 * Returns the base directory of the "core" engine that can be shared across
	 * several games or across games & mods. Shaders and base localization files
	 * e.g. reside in the engine directory.
	 *
	 * @return engine directory
	 */
	static FString EngineDir();

	/**
	* Returns the root directory for user-specific engine files. Always writable.
	*
	* @return root user directory
	*/
	static FString EngineUserDir();

	/**
	* Returns the root directory for user-specific engine files which can be shared between versions. Always writable.
	*
	* @return root user directory
	*/
	static FString EngineVersionAgnosticUserDir();

	/** 
	 * Returns the content directory of the "core" engine that can be shared across
	 * several games or across games & mods. 
	 *
	 * @return engine content directory
	 */
	static FString EngineContentDir();

	/**
	 * Returns the directory the root configuration files are located.
	 *
	 * @return root config directory
	 */
	static FString EngineConfigDir();

	/**
	 * Returns the Editor Settings directory of the engine
	 *
	 * @return Editor Settings directory.
	 */
	static FString EngineEditorSettingsDir();

	/**
	 * Returns the intermediate directory of the engine
	 *
	 * @return content directory
	 */
	static FString EngineIntermediateDir();

	/**
	 * Returns the saved directory of the engine
	 *
	 * @return Saved directory.
	 */
	static FString EngineSavedDir();

	/**
	 * Returns the plugins directory of the engine
	 *
	 * @return Plugins directory.
	 */
	static FString EnginePluginsDir();

	/**
	 * Returns the directory for default Editor UI Layout files of the engine
	 * @return Directory for default Editor UI Layout files.
	 */
	static FString EngineDefaultLayoutDir();

	/**
	 * Returns the directory for project Editor UI Layout files of the engine
	 * @return Directory for project Editor UI Layout files.
	 */
	static FString EngineProjectLayoutDir();

	/**
	 * Returns the directory for user-generated Editor UI Layout files of the engine
	 * @return Directory for user-generated Editor UI Layout files.
	 */
	static FString EngineUserLayoutDir();

	/** 
	* Returns the base directory enterprise directory.
	*
	* @return enterprise directory
	*/
	static FString EnterpriseDir();

	/**
	* Returns the enterprise plugins directory
	*
	* @return Plugins directory.
	*/
	static FString EnterprisePluginsDir();

	/**
	* Returns the enterprise FeaturePack directory
	*
	* @return FeaturePack directory.
	*/
	static FString EnterpriseFeaturePackDir();

	/**
	 * Returns the directory where engine platform extensions reside
	 *
	 * @return engine platform extensions directory
	 */
	static FString EnginePlatformExtensionsDir();

	/**
	 * Returns the directory where the project's platform extensions reside
	 *
	 * @return project platform extensions directory
	 */
	static FString ProjectPlatformExtensionsDir();

	/**
	 * Returns platform and restricted extensions that are present and valid (for platforms, it uses FDataDrivePlatformInfo to determine valid platforms, it doesn't just use what's present)
	 *
	 * @return BaseDir and usable extension directories under BaseDir (either Engine or Project)
	 */
	static TArray<FString> GetExtensionDirs(const FString& BaseDir, const FString& SubDir=FString());

	/**
	 * Returns the root directory of the engine directory tree
	 *
	 * @return Root directory.
	 */
	static FString RootDir();

	/**
	 * Returns the base directory of the current project by looking at FApp::GetProjectName().
	 * This is usually a subdirectory of the installation
	 * root directory and can be overridden on the command line to allow self
	 * contained mod support.
	 *
	 * @return base directory
	 */
	static FString ProjectDir();

	/**
	* Returns the root directory for user-specific game files.
	*
	* @return game user directory
	*/
	static FString ProjectUserDir();

	/**
	 * Returns the content directory of the current game by looking at FApp::GetProjectName().
	 *
	 * @return content directory
	 */
	static FString ProjectContentDir();

	/**
	* Returns the directory the root configuration files are located.
	*
	* @return root config directory
	*/
	static FString ProjectConfigDir();

	/**
	 * Returns the saved directory of the current game by looking at FApp::GetProjectName().
	 *
	 * @return saved directory
	 */
	static const FString& ProjectSavedDir();

	/**
	 * Returns the intermediate directory of the current game by looking at FApp::GetProjectName().
	 *
	 * @return intermediate directory
	 */
	static FString ProjectIntermediateDir();

	static FString ShaderWorkingDir();

	/**
	 * Returns the plugins directory of the current game by looking at FApp::GetProjectName().
	 *
	 * @return plugins directory
	 */
	static FString ProjectPluginsDir();

	/**
	 * Returns the mods directory of the current project by looking at FApp::GetProjectName().
	 *
	 * @return mods directory
	 */
	static FString ProjectModsDir();

	/*
	* Returns true if a writable directory for downloaded data that persists across play sessions is available
	*/
	static bool HasProjectPersistentDownloadDir();

	/*
	* Returns the writable directory for downloaded data that persists across play sessions.
	*/
	static FString ProjectPersistentDownloadDir();

	/**
	 * Returns the directory the engine uses to look for the source leaf ini files. This
	 * can't be an .ini variable for obvious reasons.
	 *
	 * @return source config directory
	 */
	static FString SourceConfigDir();

	/**
	 * Returns the directory the engine saves generated config files.
	 *
	 * @return config directory
	 */
	static FString GeneratedConfigDir();

	/**
	 * Returns the directory the engine stores sandbox output
	 *
	 * @return sandbox directory
	 */
	static FString SandboxesDir();

	/**
	 * Returns the directory the engine uses to output profiling files.
	 *
	 * @return log directory
	 */
	static FString ProfilingDir();

	/**
	 * Returns the directory the engine uses to output screenshot files.
	 *
	 * @return screenshot directory
	 */
	static FString ScreenShotDir();

	/**
	 * Returns the directory the engine uses to output BugIt files.
	 *
	 * @return screenshot directory
	 */
	static FString BugItDir();

	/**
	 * Returns the directory the engine uses to output user requested video capture files.
	 *
	 * @return Video capture directory
	 */
	static FString VideoCaptureDir();

	/**
	 * Returns the directory the engine uses to output user requested audio capture files.
	 *
	 * @return Audio capture directory
	 */
	static FString AudioCaptureDir();
	
	/**
	 * Returns the directory the engine uses to output logs. This currently can't 
	 * be an .ini setting as the game starts logging before it can read from .ini
	 * files.
	 *
	 * @return log directory
	 */
	static FString ProjectLogDir();

	/** Returns the directory for automation save files */
	static FString AutomationDir();

	/** Returns the directory for automation save files that are meant to be deleted every run */
	static FString AutomationTransientDir();

	/** Returns the directory for results of automation tests. May be deleted every run. */
	static FString AutomationReportsDir();

	/** Returns the directory for automation log files */
	static FString AutomationLogDir();

	/** Returns the directory for local files used in cloud emulation or support */
	static FString CloudDir();

	/** Returns the directory that contains subfolders for developer-specific content */
	static FString GameDevelopersDir();

	/** Returns The folder name for the developer-specific directory for the current user */
	static FString GameUserDeveloperFolderName();

	/** Returns The directory that contains developer-specific content for the current user */
	static FString GameUserDeveloperDir();

	/** Returns the directory for temp files used for diffing */
	static FString DiffDir();

	/** 
	 * Returns a list of engine-specific localization paths
	 */
	static const TArray<FString>& GetEngineLocalizationPaths();

	/** 
	 * Returns a list of editor-specific localization paths
	 */
	static const TArray<FString>& GetEditorLocalizationPaths();

	/** 
	 * Returns a list of property name localization paths
	 */
	static const TArray<FString>& GetPropertyNameLocalizationPaths();

	/** 
	 * Returns a list of tool tip localization paths
	 */
	static const TArray<FString>& GetToolTipLocalizationPaths();

	/** 
	 * Returns a list of game-specific localization paths
	 */
	static const TArray<FString>& GetGameLocalizationPaths();

	/**
	 * Get the name of the platform-specific localization sub-folder
	 */
	static FString GetPlatformLocalizationFolderName();

	/** 
	 * Returns a list of restricted/internal folder names (without any slashes) which may be tested against full paths to determine if a path is restricted or not.
	 */
	static const TArray<FString>& GetRestrictedFolderNames();

	/** 
	 * Determines if supplied path uses a restricted/internal subdirectory.	Note that slashes are normalized and character case is ignored for the comparison.
	 */
	static bool IsRestrictedPath(const FString& InPath);

	/**
	 * Returns the saved directory that is not game specific. This is usually the same as
	 * EngineSavedDir().
	 *
	 * @return saved directory
	 */
	static FString GameAgnosticSavedDir();

	/** Returns the directory where engine source code files are kept */
	static FString EngineSourceDir();

	/** Returns the directory where game source code files are kept */
	static FString GameSourceDir();

	/** Returns the directory where feature packs are kept */
	static FString FeaturePackDir();

	/**
	 * Checks whether the path to the project file, if any, is set.
	 *
	 * @return true if the path is set, false otherwise.
	 */
	static bool IsProjectFilePathSet();
	
	/**
	 * Gets the path to the project file.
	 *
	 * @return Project file path.
	 */
	static FString GetProjectFilePath();

	/**
	 * Sets the path to the project file.
	 *
	 * @param NewGameProjectFilePath - The project file path to set.
	 */
	static void SetProjectFilePath( const FString& NewGameProjectFilePath );

	/**
	 * Gets the extension for this filename.
	 *
	 * @param	bIncludeDot		if true, includes the leading dot in the result
	 *
	 * @return	the extension of this filename, or an empty string if the filename doesn't have an extension.
	 */
	static FString GetExtension( const FString& InPath, bool bIncludeDot=false );

	// Returns the filename (with extension), minus any path information.
	static FString GetCleanFilename(const FString& InPath);

	// Returns the filename (with extension), minus any path information.
	static FString GetCleanFilename(FString&& InPath);

	// Returns the same thing as GetCleanFilename, but without the extension
	static FString GetBaseFilename(const FString& InPath, bool bRemovePath=true );

	// Returns the same thing as GetCleanFilename, but without the extension
	static FString GetBaseFilename(FString&& InPath, bool bRemovePath = true);

	// Returns the path in front of the filename
	static FString GetPath(const FString& InPath);

	// Returns the path in front of the filename
	static FString GetPath(FString&& InPath);

	// Returns the leaf in the path
	static FString GetPathLeaf(const FString& InPath);

	// Returns the leaf in the path
	static FString GetPathLeaf(FString&& InPath);

	/** Changes the extension of the given filename (does nothing if the file has no extension) */
	static FString ChangeExtension(const FString& InPath, const FString& InNewExtension);

	/** Sets the extension of the given filename (like ChangeExtension, but also applies the extension if the file doesn't have one) */
	static FString SetExtension(const FString& InPath, const FString& InNewExtension);

	/** Returns true if this file was found, false otherwise */
	static bool FileExists(const FString& InPath);

	/** Returns true if this directory was found, false otherwise */
	static bool DirectoryExists(const FString& InPath);

	/** Returns true if this path represents a root drive or volume */
	static bool IsDrive(const FString& InPath);

	/** Returns true if this path is relative to another path */
	static bool IsRelative(const FString& InPath);

	/** Convert all / and \ to TEXT("/") */
	static void NormalizeFilename(FString& InPath);

	/**
	 * Checks if two paths are the same.
	 *
	 * @param PathA First path to check.
	 * @param PathB Second path to check.
	 *
	 * @returns True if both paths are the same. False otherwise.
	 */
	static bool IsSamePath(const FString& PathA, const FString& PathB);

	/** Determines if a path is under a given directory */
	static bool IsUnderDirectory(const FString& InPath, const FString& InDirectory);

	/** Normalize all / and \ to TEXT("/") and remove any trailing TEXT("/") if the character before that is not a TEXT("/") or a colon */
	static void NormalizeDirectoryName(FString& InPath);

	/**
	 * Takes a fully pathed string and eliminates relative pathing (eg: annihilates ".." with the adjacent directory).
	 * Assumes all slashes have been converted to TEXT('/').
	 * For example, takes the string:
	 *	BaseDirectory/SomeDirectory/../SomeOtherDirectory/Filename.ext
	 * and converts it to:
	 *	BaseDirectory/SomeOtherDirectory/Filename.ext
	 */
	static bool CollapseRelativeDirectories(FString& InPath);

	/**
	 * Removes duplicate slashes in paths.
	 * Assumes all slashes have been converted to TEXT('/').
	 * For example, takes the string:
	 *	BaseDirectory/SomeDirectory//SomeOtherDirectory////Filename.ext
	 * and converts it to:
	 *	BaseDirectory/SomeDirectory/SomeOtherDirectory/Filename.ext
	 */
	static void RemoveDuplicateSlashes(FString& InPath);

	 /** Returns a copy of the given path on which duplicate slashes were removed (see the inplace version for more details). */
	static FString RemoveDuplicateSlashes(const FString& InPath);

	/**
	 * Make fully standard "Unreal" pathname:
	 *    - Normalizes path separators [NormalizeFilename]
	 *    - Removes extraneous separators  [NormalizeDirectoryName, as well removing adjacent separators]
	 *    - Collapses internal ..'s
	 *    - Makes relative to Engine\Binaries\<Platform> (will ALWAYS start with ..\..\..)
	 */
	static FString CreateStandardFilename(const FString& InPath);

	static void MakeStandardFilename(FString& InPath);

	/** Takes an "Unreal" pathname and converts it to a platform filename. */
	static void MakePlatformFilename(FString& InPath);

	/** 
	 * Assuming both paths (or filenames) are relative to the same base dir, modifies InPath to be relative to InRelativeTo
	 *
	 * @param InPath Path to change to be relative to InRelativeTo
	 * @param InRelativeTo Path to use as the new relative base
	 * @returns true if InPath was changed to be relative
	 */
	static bool MakePathRelativeTo( FString& InPath, const TCHAR* InRelativeTo );

	/**
	 * Converts a relative path name to a fully qualified name relative to the process BaseDir().
	 */
	static FString ConvertRelativePathToFull(const FString& InPath);

	/**
	 * Converts a relative path name to a fully qualified name relative to the process BaseDir().
	 */
	static FString ConvertRelativePathToFull(FString&& InPath);

	/**
	 * Converts a relative path name to a fully qualified name relative to the specified BasePath.
	 */
	static FString ConvertRelativePathToFull(const FString& BasePath, const FString& InPath);

	/**
	 * Converts a relative path name to a fully qualified name relative to the specified BasePath.
	 */
	static FString ConvertRelativePathToFull(const FString& BasePath, FString&& InPath);

	/**
	 * Converts a relative path name to a fully qualified name relative to the specified BasePath.
	 */
	static FString ConvertRelativePathToFull(FString&& BasePath, const FString& InPath);

	/**
	 * Converts a relative path name to a fully qualified name relative to the specified BasePath.
	 */
	static FString ConvertRelativePathToFull(FString&& BasePath, FString&& InPath);

	/**
	 * Converts a normal path to a sandbox path (in Saved/Sandboxes).
	 *
	 * @param InSandboxName The name of the sandbox.
	 */
	static FString ConvertToSandboxPath( const FString& InPath, const TCHAR* InSandboxName );

	/**
	 * Converts a sandbox (in Saved/Sandboxes) path to a normal path.
	 *
	 * @param InSandboxName The name of the sandbox.
	 */
	static FString ConvertFromSandboxPath( const FString& InPath, const TCHAR* InSandboxName );

	/** 
	 * Creates a temporary filename with the specified prefix.
	 *
	 * @param Path The file pathname.
	 * @param Prefix The file prefix.
	 * @param Extension File extension ('.' required).
	 */
	static FString CreateTempFilename( const TCHAR* Path, const TCHAR* Prefix = TEXT(""), const TCHAR* Extension = TEXT(".tmp") );

	/**
	* Returns a string containing all invalid characters as dictated by the operating system
	*/
	static FString GetInvalidFileSystemChars();

	/**
	*	Returns a string that is safe to use as a filename because all items in
	*	GetInvalidFileSystemChars() are removed
	*/
	static FString MakeValidFileName(const FString& InString, const TCHAR InReplacementChar = TEXT('\0'));

	/** 
	 * Validates that the parts that make up the path contain no invalid characters as dictated by the operating system
	 * Note that this is a different set of restrictions to those imposed by FPackageName
	 *
	 * @param InPath - path to validate
	 * @param OutReason - optional parameter to fill with the failure reason
	 */
	static bool ValidatePath( const FString& InPath, FText* OutReason = nullptr );

	/**
	 * Parses a fully qualified or relative filename into its components (filename, path, extension).
	 *
	 * @param	InPath			[in] Full filename path
	 * @param	PathPart		[out] receives the value of the path portion of the input string
	 * @param	FilenamePart	[out] receives the value of the filename portion of the input string
	 * @param	ExtensionPart	[out] receives the value of the extension portion of the input string
	 */
	static void Split( const FString& InPath, FString& PathPart, FString& FilenamePart, FString& ExtensionPart );

	/** Gets the relative path to get from BaseDir to RootDirectory  */
	static const FString& GetRelativePathToRoot();

	template <typename... PathTypes>
	FORCEINLINE static FString Combine(PathTypes&&... InPaths)
	{
		return CombineImpl<PathTypes...>(Forward<PathTypes>(InPaths)...);
	}

	/**
	 * Frees any memory retained by FPaths.
	 */
	static void TearDown();

protected:

	static void CombineInternal(FString& OutPath, const TCHAR** Paths, int32 NumPaths);

private:
	struct FStaticData;

	template <typename... PathTypes>
	FORCEINLINE static FString CombineImpl(UE::Core::Private::TToStringType_T<std::decay_t<PathTypes>>... InPaths)
	{
		const TCHAR* Paths[] = { GetTCharPtr(InPaths)... };
		FString Out;
		
		CombineInternal(Out, Paths, UE_ARRAY_COUNT(Paths));
		return Out;
	}

	FORCEINLINE static const TCHAR* GetTCharPtr(const TCHAR* Ptr)
	{
		return Ptr;
	}

	FORCEINLINE static const TCHAR* GetTCharPtr(const FString& Str)
	{
		return *Str;
	}

	/** Returns, if any, the value of the -userdir command line argument. This can be used to sandbox artifacts to a desired location */
	static const FString& CustomUserDirArgument();

	/** Returns, if any, the value of the -shaderworkingdir command line argument. This can be used to sandbox shader working files to a desired location */
	static const FString& CustomShaderDirArgument();
};
