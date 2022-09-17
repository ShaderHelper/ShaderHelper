// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Textures/TextureAtlas.h"


DECLARE_MULTICAST_DELEGATE_OneParam(FOnReleaseFontResources, const class FSlateFontCache&);


struct SLATECORE_API FSlateFontKey
{
public:
	FSlateFontKey( const FSlateFontInfo& InInfo, const FFontOutlineSettings& InFontOutlineSettings, const float InScale )
		: FontInfo( InInfo )
		, OutlineSettings(InFontOutlineSettings)
		, Scale( InScale )
		, KeyHash( 0 )
	{
		KeyHash = HashCombine(KeyHash, GetTypeHash(FontInfo));
		KeyHash = HashCombine(KeyHash, GetTypeHash(OutlineSettings));
		KeyHash = HashCombine(KeyHash, GetTypeHash(Scale));
	}

	FORCEINLINE const FSlateFontInfo& GetFontInfo() const
	{
		return FontInfo;
	}

	FORCEINLINE float GetScale() const
	{
		return Scale;
	}

	FORCEINLINE const FFontOutlineSettings& GetFontOutlineSettings() const
	{
		return OutlineSettings;
	}

	inline bool IsIdenticalToForCaching(const FSlateFontKey& Other) const
	{
		return FontInfo.IsIdentialToForCaching(Other.FontInfo)
			&& OutlineSettings.IsIdenticalToForCaching(Other.OutlineSettings)
			&& Scale == Other.Scale;
	}

	friend inline uint32 GetTypeHash( const FSlateFontKey& Key )
	{
		return Key.KeyHash;
	}

private:
	FSlateFontInfo FontInfo;
	FFontOutlineSettings OutlineSettings;
	float Scale;
	uint32 KeyHash;
};


template<typename ValueType>
struct FSlateFontKeyFuncs : BaseKeyFuncs<TPair<FSlateFontKey, ValueType>, FSlateFontKey, false>
{
	typedef BaseKeyFuncs <
		TPair<FSlateFontKey, ValueType>,
		FSlateFontKey
	> Super;
	typedef typename Super::ElementInitType ElementInitType;
	typedef typename Super::KeyInitType     KeyInitType;

	FORCEINLINE static const FSlateFontKey& GetSetKey(ElementInitType Element)
	{
		return Element.Key;
	}

	FORCEINLINE static bool Matches(const FSlateFontKey& A, const FSlateFontKey& B)
	{
		return A.IsIdenticalToForCaching(B);
	}

	FORCEINLINE static uint32 GetKeyHash(const FSlateFontKey& Identifier)
	{
		return GetTypeHash(Identifier);
	}
};


/** Contains pixel data for a character rendered from freetype as well as measurement info */
struct FCharacterRenderData
{
	/** Raw pixels of the rendered character */
	TArray<uint8> RawPixels;
	/** Width of the character in pixels */
	int16 SizeX = 0;
	/** Height of the character in pixels */
	int16 SizeY = 0;
	/** The vertical distance from the baseline to the topmost border of the glyph bitmap */
	int16 VerticalOffset = 0;
	/** The horizontal distance from the origin to the leftmost border of the character */
	int16 HorizontalOffset = 0;
	/** True if the rendered character is 8-bit grayscale, or false if it's 8-bit per-channel BGRA color */
	bool bIsGrayscale = true;
	/** True if the rendered character supports outlines, false otherwise */
	bool bSupportsOutline = false;
};

/**
 * Interface to all Slate font textures, both atlased and non-atlased
 */
class ISlateFontTexture
{
public:
	virtual ~ISlateFontTexture() {}

	/**
	 * Returns the texture resource used by Slate
	 */
	virtual class FSlateShaderResource* GetSlateTexture() const = 0;

	/**
	 * Returns the texture resource used the Engine
	 */
	virtual class FTextureResource* GetEngineTexture() = 0;

	/**
	 * Returns whether the texture resource is 8-bit grayscale or 8-bit per-channel BGRA color
	 */
	virtual bool IsGrayscale() const = 0;

	/**
	 * Releases rendering resources of this texture
	 */
	virtual void ReleaseRenderingResources() = 0;
};

/** 
 * Representation of a texture for fonts in which characters are packed tightly based on their bounding rectangle 
 */
class SLATECORE_API FSlateFontAtlas : public ISlateFontTexture, public FSlateTextureAtlas
{
public:
	FSlateFontAtlas(uint32 InWidth, uint32 InHeight, const bool InIsGrayscale);
	virtual ~FSlateFontAtlas();

	//~ ISlateFontTexture interface
	virtual bool IsGrayscale() const override final;
	virtual FSlateShaderResource* GetAtlasTexture() const override { return GetSlateTexture(); }
	virtual void ReleaseRenderingResources() { ReleaseResources(); }

	/**
	 * Flushes all cached data.
	 */
	void Flush();

	/** 
	 * Adds a character to the texture.
	 *
	 * @param CharInfo	Information about the size of the character
	 */
	const struct FAtlasedTextureSlot* AddCharacter( const FCharacterRenderData& CharInfo );
};

class ISlateFontAtlasFactory
{
public:
	virtual FIntPoint GetAtlasSize(const bool InIsGrayscale) const = 0;
	virtual TSharedRef<FSlateFontAtlas> CreateFontAtlas(const bool InIsGrayscale) const = 0;
	virtual TSharedPtr<ISlateFontTexture> CreateNonAtlasedTexture(const uint32 InWidth, const uint32 InHeight, const bool InIsGrayscale, const TArray<uint8>& InRawData) const = 0;
};
