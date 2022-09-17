// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Serialization/BitArchive.h"
#include "Containers/Array.h"

CORE_API void appBitsCpy( uint8* Dest, int32 DestBit, uint8* Src, int32 SrcBit, int32 BitCount );

/*-----------------------------------------------------------------------------
	FBitReader.
-----------------------------------------------------------------------------*/

//@TODO: FLOATPRECISION: Public API assumes it can have > 2 GB of bits, but internally uses int32 based TArray for bytes and also has uint32 clamping on bit counts in various places

//
// Reads bitstreams.
//
struct CORE_API FBitReader : public FBitArchive
{
	friend struct FBitReaderMark;

public:
	FBitReader( uint8* Src = nullptr, int64 CountBits = 0 );

	FBitReader(const FBitReader&) = default;
    FBitReader& operator=(const FBitReader&) = default;
    FBitReader(FBitReader&&) = default;
    FBitReader& operator=(FBitReader&&) = default;

	void SetData( FBitReader& Src, int64 CountBits );
	void SetData( uint8* Src, int64 CountBits );
	void SetData( TArray<uint8>&& Src, int64 CountBits );

#if defined(_MSC_VER) && PLATFORM_DESKTOP
#pragma warning( push )
#pragma warning( disable : 4789 )	// Windows PGO (LTCG) is causing nonsensical errors in certain build environments
#endif
	FORCEINLINE_DEBUGGABLE void SerializeBits( void* Dest, int64 LengthBits )
	{
		//@TODO: FLOATPRECISION: This function/class pretends it is 64-bit aware, e.g., in the type of LengthBits and the Pos member, but it is not as appBitsCpy is only 32 bits, the inner Buffer is a 32 bit TArray, etc...
		if ( IsError() || Pos+LengthBits > Num)
		{
			if (!IsError())
			{
				SetOverflowed(LengthBits);
				//UE_LOG( LogNetSerialization, Error, TEXT( "FBitReader::SerializeBits: Pos + LengthBits > Num" ) );
			}
			FMemory::Memzero( Dest, (LengthBits+7)>>3 );
			return;
		}
		//for( int32 i=0; i<LengthBits; i++,Pos++ )
		//	if( Buffer(Pos>>3) & GShift[Pos&7] )
		//		((uint8*)Dest)[i>>3] |= GShift[i&7];
		if( LengthBits == 1 )
		{
			((uint8*)Dest)[0] = 0;
			if( Buffer[(int32)(Pos>>3)] & Shift(Pos&7) )
				((uint8*)Dest)[0] |= 0x01;
			Pos++;
		}
		else if (LengthBits != 0)
		{
			((uint8*)Dest)[((LengthBits+7)>>3) - 1] = 0;
			appBitsCpy((uint8*)Dest, 0, Buffer.GetData(), (int32)Pos, (int32)LengthBits);
			Pos += LengthBits;
		}
	}
#if defined(_MSC_VER) && PLATFORM_DESKTOP
#pragma warning( pop )
#endif

	virtual void SerializeBitsWithOffset( void* Dest, int32 DestBit, int64 LengthBits ) override;

	// OutValue < ValueMax
	FORCEINLINE_DEBUGGABLE void SerializeInt(uint32& OutValue, uint32 ValueMax)
	{
		if (!IsError())
		{
			// Use local variable to avoid Load-Hit-Store
			uint32 Value = 0;
			int64 LocalPos = Pos;
			const int64 LocalNum = Num;

			for (uint32 Mask=1; (Value + Mask) < ValueMax && Mask; Mask *= 2, LocalPos++)
			{
				if (LocalPos >= LocalNum)
				{
					SetOverflowed(LocalPos - Pos);
					break;
				}

				if (Buffer[(int32)(LocalPos >> 3)] & Shift(LocalPos & 7))
				{
					Value |= Mask;
				}
			}

			// Now write back
			Pos = LocalPos;
			OutValue = Value;
		}
	}

	virtual void SerializeIntPacked(uint32& Value) override;

	FORCEINLINE_DEBUGGABLE uint32 ReadInt(uint32 Max)
	{
		uint32 Value = 0;

		SerializeInt(Value, Max);

		return Value;
	}

	FORCEINLINE_DEBUGGABLE uint8 ReadBit()
	{
		uint8 Bit=0;
		//SerializeBits( &Bit, 1 );
		if ( !IsError() )
		{
			int64 LocalPos = Pos;
			const int64 LocalNum = Num;
			if (LocalPos >= LocalNum)
			{
				SetOverflowed(1);
				//UE_LOG( LogNetSerialization, Error, TEXT( "FBitReader::SerializeInt: LocalPos >= LocalNum" ) );
			}
			else
			{
				Bit = !!(Buffer[(int32)(LocalPos>>3)] & Shift(LocalPos&7));
				Pos++;
			}
		}
		return Bit;
	}

	FORCEINLINE_DEBUGGABLE void Serialize( void* Dest, int64 LengthBytes )
	{
		SerializeBits( Dest, LengthBytes*8 );
	}

	FORCEINLINE_DEBUGGABLE uint8* GetData()
	{
		return Buffer.GetData();
	}

	FORCEINLINE_DEBUGGABLE const uint8* GetData() const
	{
		return Buffer.GetData();
	}

	FORCEINLINE_DEBUGGABLE const TArray<uint8>& GetBuffer() const
	{
		return Buffer;
	}

	FORCEINLINE_DEBUGGABLE uint8* GetDataPosChecked()
	{
		check(Pos % 8 == 0);
		return &Buffer[(int32)(Pos >> 3)];
	}

	FORCEINLINE_DEBUGGABLE int64 GetBytesLeft() const
	{
		return ((Num - Pos) + 7) >> 3;
	}
	FORCEINLINE_DEBUGGABLE int64 GetBitsLeft() const
	{
		return (Num - Pos);
	}
	FORCEINLINE_DEBUGGABLE bool AtEnd()
	{
		return IsError() || Pos>=Num;
	}
	FORCEINLINE_DEBUGGABLE int64 GetNumBytes() const
	{
		return (Num+7)>>3;
	}
	FORCEINLINE_DEBUGGABLE int64 GetNumBits() const
	{
		return Num;
	}
	FORCEINLINE_DEBUGGABLE int64 GetPosBits() const
	{
		return Pos;
	}
	FORCEINLINE_DEBUGGABLE void EatByteAlign()
	{
		int64 PrePos = Pos;

		// Skip over remaining bits in current byte
		Pos = (Pos+7) & (~0x07);

		if ( Pos > Num )
		{
			//UE_LOG( LogNetSerialization, Error, TEXT( "FBitReader::EatByteAlign: Pos > Num" ) );
			SetOverflowed(Pos - PrePos);
		}
	}

	/**
	 * Marks this bit reader as overflowed.
	 *
	 * @param LengthBits	The number of bits being read at the time of overflow
	 */
	void SetOverflowed(int64 LengthBits);

	/** Set the stream at the end */
	void SetAtEnd() { Pos = Num; }

	void AppendDataFromChecked( FBitReader& Src );
	void AppendDataFromChecked( uint8* Src, uint32 NumBits );
	void AppendTo( TArray<uint8> &Buffer );

	/** Counts the in-memory bytes used by this object */
	virtual void CountMemory(FArchive& Ar) const;

protected:

	TArray<uint8> Buffer;
	int64 Num;
	int64 Pos;

	/** Copies version information used for network compatibility from Source to this archive */
	void SetNetVersionsFromArchive(FArchive& Source);

private:

	FORCEINLINE uint8 Shift(uint8 Cnt)
	{
		return (uint8)(1<<Cnt);
	}

};


//
// For pushing and popping FBitWriter positions.
//
struct CORE_API FBitReaderMark
{
public:

	FBitReaderMark()
		: Pos(0)
	{ }

	FBitReaderMark( FBitReader& Reader )
		: Pos(Reader.Pos)
	{ }

	FORCEINLINE_DEBUGGABLE int64 GetPos() const
	{
		return Pos;
	}

	FORCEINLINE_DEBUGGABLE void Pop( FBitReader& Reader )
	{
		Reader.Pos = Pos;
	}

	void Copy( FBitReader& Reader, TArray<uint8> &Buffer );

private:

	int64 Pos;
};
