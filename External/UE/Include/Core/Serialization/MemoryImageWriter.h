// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Serialization/MemoryLayout.h"

struct FMinimalName;
struct FScriptName;
class FMemoryImage;
class FMemoryImageSection;
class FPointerTableBase;

class CORE_API FMemoryImageWriter
{
public:
	explicit FMemoryImageWriter(FMemoryImage& InImage);
	explicit FMemoryImageWriter(FMemoryImageSection* InSection);
	~FMemoryImageWriter();

	FMemoryImage& GetImage() const;
	const FPlatformTypeLayoutParameters& GetHostLayoutParams() const;
	const FPlatformTypeLayoutParameters& GetTargetLayoutParams() const;
	FPointerTableBase& GetPointerTable() const;
	const FPointerTableBase* TryGetPrevPointerTable() const;

	inline bool Is32BitTarget() const { return GetTargetLayoutParams().Is32Bit(); }
	inline bool Is64BitTarget() const { return !Is32BitTarget(); }

	int32 AddTypeDependency(const FTypeLayoutDesc& TypeDesc);

	void WriteObject(const void* Object, const FTypeLayoutDesc& TypeDesc);
	void WriteObjectArray(const void* Object, const FTypeLayoutDesc& TypeDesc, uint32_t NumArray);
	void WriteRootObject(const void* Object, const FTypeLayoutDesc& TypeDesc);

	uint32 GetOffset() const;
	uint32 WriteAlignment(uint32 Alignment);
	void WritePaddingToSize(uint32 Offset);
	uint32 WriteBytes(const void* Data, uint32 Size);
	FMemoryImageWriter WritePointer(const FTypeLayoutDesc& StaticTypeDesc, const FTypeLayoutDesc& DerivedTypeDesc, uint32* OutOffsetToBase = nullptr);
	FMemoryImageWriter WritePointer(const FTypeLayoutDesc& TypeDesc);
	uint32 WriteNullPointer();
	uint32 WriteRawPointerSizedBytes(uint64 PointerValue);
	uint32 WriteVTable(const FTypeLayoutDesc& TypeDesc, const FTypeLayoutDesc& DerivedTypeDesc);
	uint32 WriteFName(const FName& Name);
	uint32 WriteFMinimalName(const FMinimalName& Name);
	uint32 WriteFScriptName(const FScriptName& Name);

	template<typename T>
	void WriteObject(const T& Object)
	{
		const FTypeLayoutDesc& TypeDesc = GetTypeLayoutDesc(TryGetPrevPointerTable(), Object);
		WriteObject(&Object, TypeDesc);
	}

	template<typename T>
	void WriteRootObject(const T& Object)
	{
		const FTypeLayoutDesc& TypeDesc = GetTypeLayoutDesc(TryGetPrevPointerTable(), Object);
		WriteRootObject(&Object, TypeDesc);
	}

	template<typename T>
	uint32 WriteAlignment()
	{
		return WriteAlignment(alignof(T));
	}

	template<typename T>
	uint32 WriteBytes(const T& Data)
	{
		return WriteBytes(&Data, sizeof(T));
	}

//private:
	FMemoryImageSection* Section;
};

class FMemoryUnfreezeContent
{
public:
	explicit FMemoryUnfreezeContent(const FPointerTableBase* InPointerTable)
		: PrevPointerTable(InPointerTable)
		, bIsFrozenForCurrentPlatform(true)
	{
		FrozenLayoutParameters.InitializeForCurrent();
	}

	FMemoryUnfreezeContent(const FPointerTableBase* InPointerTable, const FPlatformTypeLayoutParameters& InLayoutParams)
		: PrevPointerTable(InPointerTable)
		, FrozenLayoutParameters(InLayoutParams)
		, bIsFrozenForCurrentPlatform(InLayoutParams.IsCurrentPlatform())
	{
	}

	inline const FPointerTableBase* TryGetPrevPointerTable() const { return PrevPointerTable; }

	CORE_API const FTypeLayoutDesc* GetDerivedTypeDesc(const FTypeLayoutDesc& StaticTypeDesc, int32 TypeIndex) const;

	inline uint32 UnfreezeObject(const void* Object, const FTypeLayoutDesc& TypeDesc, void* OutDst) const
	{
		return TypeDesc.UnfrozenCopyFunc(*this, Object, TypeDesc, OutDst);
	}

	template<typename T>
	inline uint32 UnfreezeObject(const T& Object, void* OutDst) const
	{
		const FTypeLayoutDesc& TypeDesc = GetTypeLayoutDesc(TryGetPrevPointerTable(), Object);
		return UnfreezeObject(&Object, TypeDesc, OutDst);
	}

	const FPointerTableBase* PrevPointerTable = nullptr;

	// Layout of the frozen data
	FPlatformTypeLayoutParameters FrozenLayoutParameters;
	bool bIsFrozenForCurrentPlatform; // FrozenLayoutParameters.IsCurrentPlatform
};
