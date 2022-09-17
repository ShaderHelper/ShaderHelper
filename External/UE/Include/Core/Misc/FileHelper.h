// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/StringView.h"
#include "Containers/UnrealString.h"
#include "HAL/FileManager.h"
#include "Containers/ArrayView.h"
#include "Misc/EnumClassFlags.h"
#include "Math/Color.h"

/*-----------------------------------------------------------------------------
	FFileHelper
-----------------------------------------------------------------------------*/
struct CORE_API FFileHelper
{
	enum class EHashOptions
	{
		None                =0,
		/** Enable the async task for verifying the hash for the file being loaded */
		EnableVerify		=1<<0,
		/** A missing hash entry should trigger an error */
		ErrorMissingHash	=1<<1
	};

	enum class EEncodingOptions
	{
		AutoDetect,
		ForceAnsi,
		ForceUnicode,
		ForceUTF8,
		ForceUTF8WithoutBOM
	};

	enum class EColorChannel
	{
		R,
		G,
		B,
		A,
		All
	};

	/**
	 * Load a text file to an FString.
	 * Supports all combination of ANSI/Unicode files and platforms.
	*/
	static void BufferToString( FString& Result, const uint8* Buffer, int32 Size );

	/**
	 * Load a binary file to a dynamic array with two uninitialized bytes at end as padding.
	 *
	 * @param Result    Receives the contents of the file
	 * @param Filename  The file to read
	 * @param Flags     Flags to pass to IFileManager::CreateFileReader
	*/
	static bool LoadFileToArray( TArray<uint8>& Result, const TCHAR* Filename, uint32 Flags = 0 );

	/**
	 * Load a binary file to a dynamic array with two uninitialized bytes at end as padding.
	 *
	 * @param Result    Receives the contents of the file
	 * @param Filename  The file to read
	 * @param Flags     Flags to pass to IFileManager::CreateFileReader
	*/
	static bool LoadFileToArray( TArray64<uint8>& Result, const TCHAR* Filename, uint32 Flags = 0 );

	/**
	 * Load a text file to an FString. Supports all combination of ANSI/Unicode files and platforms.
	 *
	 * @param Result       String representation of the loaded file
	 * @param Archive      Name of the archive to load from
	 * @param VerifyFlags  Flags controlling the hash verification behavior ( see EHashOptions )
	 */
	static bool LoadFileToString(FString& Result, FArchive& Reader, EHashOptions VerifyFlags = EHashOptions::None);

	/**
	 * Load a text file to an FString. Supports all combination of ANSI/Unicode files and platforms.
	 *
	 * @param Result       String representation of the loaded file
	 * @param Filename     Name of the file to load
	 * @param VerifyFlags  Flags controlling the hash verification behavior ( see EHashOptions )
	 */
	static bool LoadFileToString( FString& Result, const TCHAR* Filename, EHashOptions VerifyFlags = EHashOptions::None, uint32 ReadFlags = 0 );

	/**
	 * Load a text file to an FString. Supports all combination of ANSI/Unicode files and platforms.
	 *
	 * @param Result       String representation of the loaded file
	 * @param PlatformFile PlatformFile interface to use
	 * @param Filename     Name of the file to load
	 * @param VerifyFlags  Flags controlling the hash verification behavior ( see EHashOptions )
	 */
	static bool LoadFileToString(FString& Result, IPlatformFile* PlatformFile, const TCHAR* Filename, EHashOptions VerifyFlags = EHashOptions::None, uint32 ReadFlags = 0);

	/**
	 * Load a text file to an array of strings. Supports all combination of ANSI/Unicode files and platforms.
	 *
	 * @param Result       String representation of the loaded file
	 * @param Filename     Name of the file to load
	 */
	static bool LoadFileToStringArray( TArray<FString>& Result, const TCHAR* Filename );

	UE_DEPRECATED(4.26, "LoadFileToStringArray no longer supports VerifyFlags. You can use UE::String::ParseLines to split up a string loaded with LoadFileToString")
	static bool LoadFileToStringArray(TArray<FString>& Result, const TCHAR* Filename, EHashOptions VerifyFlags);

	/**
	 * Load a text file to an array of strings, filtered by a user-defined predicate. Supports all combination of ANSI/Unicode files and platforms.
	 *
	 * @param Result       String representation of the loaded file
	 * @param Filename     Name of the file to load
	 * @param Predicate    Condition for whether or not to add the line to the array
	 */
	static bool LoadFileToStringArrayWithPredicate(TArray<FString>& Result, const TCHAR* Filename, TFunctionRef<bool(const FString&)> Predicate);

	UE_DEPRECATED(4.26, "LoadFileToStringArrayWithPredicate no longer supports VerifyFlags. You can use UE::String::ParseLines to split up a string loaded with LoadFileToString")
	static bool LoadFileToStringArrayWithPredicate(TArray<FString>& Result, const TCHAR* Filename, TFunctionRef<bool(const FString&)> Predicate, EHashOptions VerifyFlags);

	/**
	 * Load a text file and invoke a visitor for each line. Supports all combination of ANSI/Unicode files and platforms.
	 *
	 * @param Filename     Name of the file to load
	 * @param Visitor      Visitor to invoke for each non-empty line in the file
	 */
	static bool LoadFileToStringWithLineVisitor(const TCHAR* Filename, TFunctionRef<void(FStringView Line)> Visitor);

	/**
	 * Save a binary array to a file.
	 */
	static bool SaveArrayToFile(TArrayView<const uint8> Array, const TCHAR* Filename, IFileManager* FileManager=&IFileManager::Get(), uint32 WriteFlags = 0);

	/**
	 * Save a binary array to a file.
	 */
	static bool SaveArrayToFile( const TArray64<uint8>& Array, const TCHAR* Filename, IFileManager* FileManager = &IFileManager::Get(), uint32 WriteFlags = 0 );

	/**
	 * Write the FString to a file.
	 * Supports all combination of ANSI/Unicode files and platforms.
	 */
	static bool SaveStringToFile( FStringView String, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect, IFileManager* FileManager = &IFileManager::Get(), uint32 WriteFlags = 0 );

	/**
	 * Write the FString to a file.
	 * Supports all combination of ANSI/Unicode files and platforms.
	 */
	static bool SaveStringArrayToFile( const TArray<FString>& Lines, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect, IFileManager* FileManager = &IFileManager::Get(), uint32 WriteFlags = 0 );

	/**
	 * Saves a 24/32Bit BMP file to disk
	 * 
	 * @param Pattern filename with path, must not be 0, if with "bmp" extension (e.g. "out.bmp") the filename stays like this, if without (e.g. "out") automatic index numbers are addended (e.g. "out00002.bmp")
	 * @param DataWidth - Width of the bitmap supplied in Data >0
	 * @param DataHeight - Height of the bitmap supplied in Data >0
	 * @param Data must not be 0
	 * @param SubRectangle optional, specifies a sub-rectangle of the source image to save out. If NULL, the whole bitmap is saved
	 * @param FileManager must not be 0
	 * @param OutFilename optional, if specified filename will be output
	 * @param bInWriteAlpha optional, specifies whether to write out the alpha channel. Will force BMP V4 format.
	 * @param ColorChannel optional, specifies a specific channel to write out (will be written out to all channels gray scale).
	 *
	 * @return true if success
	 */
	static bool CreateBitmap( const TCHAR* Pattern, int32 DataWidth, int32 DataHeight, const struct FColor* Data, struct FIntRect* SubRectangle = NULL, IFileManager* FileManager = &IFileManager::Get(), FString* OutFilename = NULL, bool bInWriteAlpha = false, EColorChannel ColorChannel = EColorChannel::All);

	/**
	 * Generates the next unique bitmap filename with a specified extension
	 *
	 * @param Pattern		Filename with path, but without extension.
	 * @param Extension		File extension to be appended
	 * @param OutFilename	Reference to an FString where the newly generated filename will be placed
	 * @param FileManager	Reference to a IFileManager (or the global instance by default)
	 *
	 * @return true if success
	 */
	static bool GenerateNextBitmapFilename(const FString& Pattern, const FString& Extension, FString& OutFilename, IFileManager* FileManager = &IFileManager::Get());
	
	/**
	 * Generates the next unique bitmap filename with a specified extension
	 *
	 * @param Pattern		Filename with path, but without extension.
	 * @param Extension		File extension to be appended
	 * @param OutFilename	Reference to an FString where the newly generated filename will be placed
	 *
	 * @return true if success
	 */
	static void GenerateDateTimeBasedBitmapFilename(const FString& Pattern, const FString& Extension, FString& OutFilename);
	
	/**
	 *	Load the given ANSI text file to an array of strings - one FString per line of the file.
	 *	Intended for use in simple text parsing actions
	 *
	 *	@param	InFilename			The text file to read, full path
	 *	@param	InFileManager		The filemanager to use - NULL will use &IFileManager::Get()
	 *	@param	OutStrings			The array of FStrings to fill in
	 *
	 *	@return	bool				true if successful, false if not
	 */
	static bool LoadANSITextFileToStrings(const TCHAR* InFilename, IFileManager* InFileManager, TArray<FString>& OutStrings);

	/**
	* Checks to see if a filename is valid for saving.
	* A filename must be under FPlatformMisc::GetMaxPathLength() to be saved
	*
	* @param Filename	Filename, with or without path information, to check.
	* @param OutError	If an error occurs, this is the reason why
	*/
	static bool IsFilenameValidForSaving(const FString& Filename, FText& OutError);

	enum class UE_DEPRECATED(5.0, "EChannelMask has been deprecated in favor of EColorChannel") EChannelMask
	{
		R = STRUCT_OFFSET(FColor, R),
		G = STRUCT_OFFSET(FColor, G),
		B = STRUCT_OFFSET(FColor, B),
		A = STRUCT_OFFSET(FColor, A),
		All = R | G | B | A
	};

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UE_DEPRECATED(5.0, "EChannelMask has been deprecated in favor of EColorChannel, please use the other CreateBitmap() method.")
	static bool CreateBitmap(const TCHAR* Pattern, int32 DataWidth, int32 DataHeight, const struct FColor* Data, struct FIntRect* SubRectangle, IFileManager* FileManager, FString* OutFilename, bool bInWriteAlpha, EChannelMask ChannelMask);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
};

ENUM_CLASS_FLAGS(FFileHelper::EHashOptions)
