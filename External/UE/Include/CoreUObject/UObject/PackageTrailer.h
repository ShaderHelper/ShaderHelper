// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Compression/CompressedBuffer.h"
#include "Containers/Map.h"
#include "IO/IoHash.h"
#include "UObject/NameTypes.h"

class FArchive;
class FLinkerSave;
class FPackagePath;

namespace UE
{

/** Trailer Format
 * The FPackageTrailer is a container that will commonly be appended to the end of a package file. The main purpose of the trailer is to store
 * the bulkdata payloads contained by the package until they are virtualized or moved to an additional storage location.
 * 
 * By storing the payloads in a data format adjacent to the rest of the package we can perform the virtualization process without needing to 
 * re-save the package itself which in turn should allow for external tools to be able to perform the virtualization process themselves
 * rather than needing to force it through engine code.
 * 
 * The package trailer is intended to an easy format for external code/script to be able to manipulate. To make things clearer we do not 
 * serialize containers directly but write out each data structure one at a time so that it should be easy to see how to manipulate the file.
 * 
 * The file is split into three parts:
 * 
 * [Header]
 * The header contains the useful info about the trailer and the payloads in general. @See UE::FLookupTableEntry for details about 
 * the look up table's data.
 * 
 * [Payload Data]
 * If the trailer is in the workspace domain package then we will store all non-virtualized payloads here. If the trailer is in the editor 
 * domain then there will be no payload data section and the header will be referencing the trailer in the workspace domain instead.
 * 
 * [Footer]
 * The footer allows for us to load the trailer in reverse and replicates the end of package file tag (PACKAGE_FILE_TAG), it should only be
 * used for finding the start of the trailer or validation.
 * 
 * CurrentVersion UE::EPackageTrailerVersion::INITIAL
 * ______________________________________________________________________________________________________________________________________________
 * | [Header]																																	|
 * | Tag				| uint64			| Should match FHeader::HeaderTag, used to identify that the data being read is an FPackageTrailer	|
 * | Version			| uint32			| Version number of the format@see UE::EPackageTrailerVersion										|
 * | HeaderLength		| uint32			| The total size of the header on disk in bytes.													|
 * | PayloadsDataLength	| uint64			| The total size of the payload data on disk in bytes												|
 * | NumPayloads		| int32				| The number of payloads in LookupTableArray														|
 * | LookupTableArray	| FLookupTableEntry | An array of FLookupTableEntry @see UE::Private::FLookupTableEntry									|
 * |____________________________________________________________________________________________________________________________________________|
 * | [Payload Data]																																|
 * | Array				| FCompressedBuffer | A binary blob containing all of the payloads. Individual payloads can be found via				|
 * |										 the LookupTableArray found in the header.															|
 * |____________________________________________________________________________________________________________________________________________|
 * | [Footer]																																	|
 * | Tag				| uint64			| Should match FFooter::FooterTag, used to identify that the data being read is an FPackageTrailer	|
 * | TrailerLength		| uint64			| The total size of the trailer on disk in bytes. Can be used to find the start of the trailer when	|
 * |										  reading backwards.																				|
 * | PackageTag			| uint32			| The end of package tag, PACKAGE_FILE_TAG. This is used to validate that a package file on disk is	|
 * |										  not corrupt. By ending the trailer with this tag we allow that validation code to work.			|
 * |____________________________________________________________________________________________________________________________________________|
 */

 /** Used to filter requests to a specific type of payload */
enum class EPayloadFilter
{
	/** All payload types. */
	All,
	/** All payloads stored locally in the package trailer. */
	Local,
	/** All payloads that are a reference to payloads stored in the workspace domain trailer*/
	Referenced,
	/** All payloads stored in a virtualized backend. */
	Virtualized
};

/** Used to show the status of a payload */
enum class EPayloadStatus
{
	/** The payload is not registered in the package trailer */
	NotFound = 0,
	/** The payload is stored locally inside the current package trailer where ever that is written to disk */
	StoredLocally,
	/** The payload is stored in the workspace domain trailer */
	StoredAsReference,
	/** The payload is virtualized and needs to be accessed via the IVirtualizationSystem */
	StoredVirtualized,
};

/** Lists the various methods of payload access that the trailer supports */
enum class EPayloadAccessMode : uint8
{
	/** The payload is stored in the Payload Data segment of the trailer and the offsets in FLookupTableEntry will be relative to the start of this segment */
	Local = 0,
	/** The payload is stored in another package trailer (most likely the workspace domain package file) and the offsets in FLookupTableEntry are absolute offsets in that external file */
	Referenced,
	/** The payload is virtualized and needs to be accessed via IVirtualizationSystem */
	Virtualized
};

/** Flags that can be set on payloads in a payload trailer */
enum class EPayloadFlags : uint32
{
	/** No flags are set */
	None
};

enum class EPackageTrailerVersion : uint32;

namespace Private
{

struct FLookupTableEntry
{
	/** Size of the entry when serialized to disk in bytes */
	static constexpr uint32 SizeOnDisk = 49;	// Identifier		| 20 bytes
												// OffsetInFile		| 8 bytes
												// CompressedSize	| 8 bytes
												// RawSize			| 8 bytes
												// Flags			| 4 bytes
												// AccessMode		| 1 byte

	FLookupTableEntry() = default;
	FLookupTableEntry(const FIoHash& InIdentifier, uint64 InRawSize);

	void Serialize(FArchive& Ar, EPackageTrailerVersion PackageTrailerVersion);
	
	[[nodiscard]] bool IsVirtualized() const
	{
		return AccessMode == EPayloadAccessMode::Virtualized;
	}

	/** Identifier for the payload */
	FIoHash Identifier;
	/** The offset into the file where we can find the payload, note that a virtualized payload will have an offset of INDEX_NONE */
	int64 OffsetInFile = INDEX_NONE;
	/** The size of the payload when compressed. This will be the same value as RawSize if the payload is not compressed */
	uint64 CompressedSize = INDEX_NONE;
	/** The size of the payload when uncompressed. */
	uint64 RawSize = INDEX_NONE;
	/** Bitfield of flags, see @UE::EPayloadFlags */
	EPayloadFlags Flags = EPayloadFlags::None;

	EPayloadAccessMode AccessMode = EPayloadAccessMode::Local;
};

} // namespace Private

/** 
 * This class is used to build a FPackageTrailer and write it disk.
 * 
 * While saving a package, payloads should be added to a FPackageTrailer via ::AddPayload then once
 * the package has been saved to disk ::BuildAndAppendTrailer should be called. 
 */
class COREUOBJECT_API FPackageTrailerBuilder
{
public:
	using AdditionalDataCallback = TFunction<void(FLinkerSave& LinkerSave, const class FPackageTrailer& Trailer)>;

	/**
	 * Creates a builder from a pre-existing FPackageTrailer.
	 * Payloads stored locally in the source trailer will be loaded from disk via the provided archive so that the
	 * builder can write them to any future trailer that it creates.
	 * 
	 * @param Trailer		The trailer to create the builder from
	 * @param Ar			An archive that the trailer can use to load payloads from 
	 * @param PackageName	The name of the package that owns the trailer. Used for error messages.
	 */
	[[nodiscard]] static FPackageTrailerBuilder CreateFromTrailer(const class FPackageTrailer& Trailer, FArchive& Ar, const FName& PackageName);

	/**
	 * Creates a builder from a pre-existing FPackageTrailer that will will reference the local payloads of the
	 * source trailer. 
	 * This means that there is no need to load the payloads.
	 *
	 * @param Trailer		The trailer to create the reference from.
	 * @param PackageName	The name of the package that owns the trailer. Used for error messages.
	 */
	[[nodiscard]] static TUniquePtr<UE::FPackageTrailerBuilder> CreateReferenceToTrailer(const class FPackageTrailer& Trailer, const FName& PackageName);

	FPackageTrailerBuilder() = delete;
	FPackageTrailerBuilder(const FName& InPackageName);
	~FPackageTrailerBuilder() = default;

	// Methods that can be called while building the trailer

	/**
	 * Adds a payload to the builder to be written to the trailer. Duplicate payloads will be discarded and only a 
	 * single instance stored in the trailer.
	 * 
	 * @param Identifier	The identifier of the payload
	 * @param Payload		The payload data
	 * @param Callback		This callback will be invoked once the FPackageTrailer has been built and appended to disk.
	 */
	void AddPayload(const FIoHash& Identifier, FCompressedBuffer Payload, AdditionalDataCallback&& Callback);

	/**
	 * Adds an already virtualized payload to the builder to be written to the trailer. When the trailer is written
	 * the payload will have EPayloadAccessMode::Virtualized set as it's access mode. It is assumed that the payload
	 * is already stored in the virtualization backends and it is up to the calling code to confirm this.
	 * Duplicate payloads will be discarded and only a single instance stored in the trailer.
	 * 
	 * @param Identifier	The identifier of the payload
	 * @param RawSize		The size of the payload (in bytes) when uncompressed
	 */
	void AddVirtualizedPayload(const FIoHash& Identifier, int64 RawSize);
	
	/**
	 * @param ExportsArchive	The linker associated with the package being written to disk.
	 * @param DataArchive		The archive where the package data has been written to. This is where the FPackageTrailer will be written to
	 * @return True if the builder was created and appended successfully and false if any error was encountered
	 */
	[[nodiscard]] bool BuildAndAppendTrailer(FLinkerSave* Linker, FArchive& DataArchive);
	
	/** Returns if the builder has any payload entries or not */
	[[nodiscard]] bool IsEmpty() const;

	[[nodiscard]] bool IsLocalPayloadEntry(const FIoHash& Identifier) const;
	[[nodiscard]] bool IsReferencedPayloadEntry(const FIoHash& Identifier) const;
	[[nodiscard]] bool IsVirtualizedPayloadEntry(const FIoHash& Identifier) const;

	/** Returns the total number of payload entries in the builder */
	[[nodiscard]] int32 GetNumPayloads() const;
	
	/** Returns the number of payload entries in the builder with the access mode EPayloadAccessMode::Local */
	[[nodiscard]] int32 GetNumLocalPayloads() const;
	/** Returns the number of payload entries in the builder with the access mode EPayloadAccessMode::Referenced */
	[[nodiscard]] int32 GetNumReferencedPayloads() const;
	/** Returns the number of payload entries in the builder with the access mode EPayloadAccessMode::Virtualized */
	[[nodiscard]] int32 GetNumVirtualizedPayloads() const;

private:
	
	/** All of the data required to add a payload that is stored locally within the trailer */
	struct LocalEntry
	{
		LocalEntry() = default;
		LocalEntry(FCompressedBuffer&& InPayload)
			: Payload(InPayload)
		{

		}
		~LocalEntry() = default;

		FCompressedBuffer Payload;
	};

	/** All of the data required to add a reference to a payload stored in another trailer */
	struct ReferencedEntry
	{
		ReferencedEntry() = default;
		ReferencedEntry(int64 InOffset, int64 InCompressedSize, int64 InRawSize)
			: Offset(InOffset)
			, CompressedSize(InCompressedSize)
			, RawSize(InRawSize)
		{

		}
		~ReferencedEntry() = default;

		int64 Offset = INDEX_NONE;
		int64 CompressedSize = INDEX_NONE;
		int64 RawSize = INDEX_NONE;
	};


	/** All of the data required to add a payload that is virtualized */
	struct VirtualizedEntry
	{
		VirtualizedEntry() = default;
		VirtualizedEntry(int64 InRawSize)
			: RawSize(InRawSize)
		{

		}
		~VirtualizedEntry() = default;

		int64 RawSize = INDEX_NONE;
	};

	// Members used when building the trailer

	/** Name of the package the trailer is being built for, used to give meaningful error messages */
	FName PackageName;

	/** Payloads that will be stored locally when the trailer is written to disk */
	TMap<FIoHash, LocalEntry> LocalEntries;
	/** Payloads that reference entries in another trailer */
	TMap<FIoHash, ReferencedEntry> ReferencedEntries;
	/** Payloads that are already virtualized and so will not be written to disk */
	TMap<FIoHash, VirtualizedEntry> VirtualizedEntries;
	/** Callbacks to invoke once the trailer has been written to the end of a package */
	TArray<AdditionalDataCallback> Callbacks;
};

/** 
 *
 * The package trailer should only ever stored the payloads in the workspace domain. If the package trailer is in the editor
 * domain then it's values should be valid, but when loading non-virtualized payloads they need to come from the workspace 
 * domain package.
 */
class COREUOBJECT_API FPackageTrailer
{
public:
	/** 
	 * Returns if the feature is enabled or disabled.
	 * 
	 * Note that this is for development purposes only and should ship as always enabled!
	 */
	[[nodiscard]] static bool IsEnabled();

	/** Try to load a trailer from a given package path. Note that it will always try to load the trailer from the workspace domain */
	[[nodiscard]] static bool TryLoadFromPackage(const FPackagePath& PackagePath, FPackageTrailer& OutTrailer);

	FPackageTrailer() = default;
	~FPackageTrailer() = default;

	FPackageTrailer(const FPackageTrailer& Other) = default;
	FPackageTrailer& operator=(const FPackageTrailer & Other) = default;

	FPackageTrailer(FPackageTrailer&& Other) = default;
	FPackageTrailer& operator=(FPackageTrailer&& Other) = default;

	/** 
	 * Serializes the trailer from the given archive assuming that the seek position of the archive is already at the correct position
	 * for the trailer.
	 * 
	 * @param Ar	The archive to load the trailer from
	 * @return		True if a valid trailer was found and was able to be loaded, otherwise false.
	 */
	[[nodiscard]] bool TryLoad(FArchive& Ar);

	/** 
	 * Serializes the trailer from the given archive BUT assumes that the seek position of the archive is at the end of the trailer
	 * and so will attempt to read the footer first and use that to find the start of the trailer in order to read the header.
	 * 
	 * @param Ar	The archive to load the trailer from
	 * @return		True if a valid trailer was found and was able to be loaded, otherwise false.
	 */
	[[nodiscard]] bool TryLoadBackwards(FArchive& Ar);

	/** 
	 * Loads a payload that is stored locally within the package trailer. Payloads stored externally (either referenced
	 * or virtualized) will not load.
	 * 
	 * @param Id The payload to load
	 * @param Ar The archive from which the payload trailer was also loaded from
	 * 
	 * @return	The payload in the form of a FCompressedBuffer. If the payload does not exist in the trailer or is not
	 *			stored locally in the trailer then the FCompressedBuffer will be null.
	 */
	[[nodiscard]] FCompressedBuffer LoadLocalPayload(const FIoHash& Id, FArchive& Ar) const;

	/** 
	 * Calling this indicates that the payload has been virtualized and will no longer be stored on disk. 
	 * 
	 * @param Identifier The payload that has been virtualized
	 * @return True if the payload was in the trailer, otherwise false
	 */
	[[nodiscard]] bool UpdatePayloadAsVirtualized(const FIoHash& Identifier);

	/** Attempt to find the status of the given payload. @See EPayloadStatus */
	[[nodiscard]] EPayloadStatus FindPayloadStatus(const FIoHash& Id) const;

	/** Returns the absolute offset of the payload in the package file, invalid and virtualized payloads will return INDEX_NONE */
	[[nodiscard]] int64 FindPayloadOffsetInFile(const FIoHash& Id) const;

	/** Returns the total size of the of the trailer on disk in bytes */
	[[nodiscard]] int64 GetTrailerLength() const;

	/** Returns an array of the payloads that match the given filter type. @See EPayloadType */
	[[nodiscard]] TArray<FIoHash> GetPayloads(EPayloadFilter Type) const;

	/** Returns the number of payloads that the trailer owns that match the given filter type. @See EPayloadType */
	[[nodiscard]] int32 GetNumPayloads(EPayloadFilter Type) const;

	struct FHeader
	{
		/** Unique value used to identify the header */
		static constexpr uint64 HeaderTag = 0xD1C43B2E80A5F697;

		/** 
		 * Size of the static header data when serialized to disk in bytes. Note that we still need to 
		 * add the size of the data in PayloadLookupTable to get the final header size on disk 
		 */
		static constexpr uint32 StaticHeaderSizeOnDisk = 28;	// HeaderTag			| 8 bytes
																// Version				| 4 bytes
																// HeaderLength			| 4 bytes
																// PayloadsDataLength	| 8 bytes
																// NumPayloads			| 4 bytes

		/** Expected tag at the start of the header */
		uint64 Tag = 0;
		/** Version of the header */
		int32 Version = INDEX_NONE;
		/** Total length of the header on disk in bytes */
		uint32 HeaderLength = 0;
		/** Total length of the payloads on disk in bytes */
		uint64 PayloadsDataLength = 0;
		/** Lookup table for the payloads on disk */
		TArray<Private::FLookupTableEntry> PayloadLookupTable;

		/** Serialization operator */
		friend FArchive& operator<<(FArchive& Ar, FHeader& Header);
	};

	struct FFooter
	{
		/** Unique value used to identify the footer */
		static constexpr uint64 FooterTag = 0x29BFCA045138DE76;

		/** Size of the footer when serialized to disk in bytes */
		static constexpr uint32 SizeOnDisk = 20;	// Tag				| 8 bytes
													// TrailerLength	| 8 bytes
													// PackageTag		| 4 bytes

		/** Expected tag at the start of the footer */
		uint64 Tag = 0;
		/** Total length of the trailer on disk in bytes */
		uint64 TrailerLength = 0;	
		/** End the trailer with PACKAGE_FILE_TAG, which we expect all package files to end with */
		uint32 PackageTag = 0;

		/** Serialization operator */
		friend FArchive& operator<<(FArchive& Ar, FFooter& Footer);
	};

private:
	friend class FPackageTrailerBuilder;

	/** Create a valid footer for the current trailer */
	FFooter CreateFooter() const;

	/** Where in the workspace domain package file the trailer is located */
	int64 TrailerPositionInFile = INDEX_NONE;

	/** 
	 * The header of the trailer. Since this contains the lookup table for payloads we keep this in memory once the trailer
	 * has been loaded. There is no need to keep the footer in memory */
	FHeader Header;
};

/**
 * Used to find the identifiers of the payload in a given package. Note that this will return the payloads included in the
 * package on disk and will not take into account any edits to the package if they are in memory and unsaved.
 *
 * @param PackagePath	The package to look in.
 * @param Filter		What sort of payloads should be returned. @see EPayloadType
 * @param OutPayloadIds	This array will be filled with the FPayloadId values found in the package that passed the filter.
 *						Note that existing content in the array will be preserved. It is up to the caller to empty it.
 *
 * @return 				True if the package was parsed successfully (although it might not have contained any payloads) and false if opening or parsing the package file failed.
 */
[[nodiscard]] COREUOBJECT_API bool FindPayloadsInPackageFile(const FPackagePath& PackagePath, EPayloadFilter Filter, TArray<FIoHash>& OutPayloadIds);

} //namespace UE