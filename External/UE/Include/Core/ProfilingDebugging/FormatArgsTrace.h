// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/CString.h"
#include "Templates/IsFloatingPoint.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/UnrealTemplate.h"
#include "Traits/IsCharType.h"

struct FFormatArgsTrace
{
	enum EFormatArgTypeCode
	{
		FormatArgTypeCode_CategoryBitShift = 6,
		FormatArgTypeCode_SizeBitMask = (1 << FormatArgTypeCode_CategoryBitShift) - 1,
		FormatArgTypeCode_CategoryBitMask = ~FormatArgTypeCode_SizeBitMask,
		FormatArgTypeCode_CategoryInteger = 1 << FormatArgTypeCode_CategoryBitShift,
		FormatArgTypeCode_CategoryFloatingPoint = 2 << FormatArgTypeCode_CategoryBitShift,
		FormatArgTypeCode_CategoryString = 3 << FormatArgTypeCode_CategoryBitShift,
	};

	template <int BufferSize, typename... Types>
	static uint16 EncodeArguments(uint8(&Buffer)[BufferSize], Types... FormatArgs)
	{
		static_assert(BufferSize < 65536, "Maximum buffer size of 16 bits exceeded");
		uint64 FormatArgsCount = sizeof...(FormatArgs);
		if (FormatArgsCount >= 256)
		{
			// Nope
			return 0;
		}
		uint64 FormatArgsSize = 1 + FormatArgsCount + GetArgumentsEncodedSize(FormatArgs...);
		if (FormatArgsSize > BufferSize)
		{
			// Nope
			return 0;
		}
		uint8* TypeCodesBufferPtr = Buffer;
		*TypeCodesBufferPtr++ = uint8(FormatArgsCount);
		uint8* PayloadBufferPtr = TypeCodesBufferPtr + FormatArgsCount;
		EncodeArgumentsInternal(TypeCodesBufferPtr, PayloadBufferPtr, FormatArgs...);
		check(PayloadBufferPtr - Buffer == FormatArgsSize);
		return (uint16)FormatArgsSize;
	}

private:
	template <typename T>
	struct TIsStringArgument
	{
		enum { Value = TAnd<TIsPointer<T>, TIsCharType<typename TRemoveCV<typename TRemovePointer<T>::Type>::Type>>::Value };
	};

	template <typename T>
	constexpr static typename TEnableIf<!TIsStringArgument<T>::Value, uint64>::Type GetArgumentEncodedSize(T Argument)
	{
		return sizeof(T);
	}

	template <typename T, typename CharType = typename TRemoveCV<typename TRemovePointer<T>::Type>::Type>
	static typename TEnableIf<TIsStringArgument<T>::Value, uint64>::Type GetArgumentEncodedSize(T Argument)
	{
		if (Argument != nullptr)
		{
			return (TCString<CharType>::Strlen(Argument) + 1) * sizeof(CharType);
		}
		else
		{
			return sizeof(CharType);
		}
	}

	constexpr static uint64 GetArgumentsEncodedSize()
	{
		return 0;
	}

	template <typename T, typename... Types>
	static uint64 GetArgumentsEncodedSize(T Head, Types... Tail)
	{
		return GetArgumentEncodedSize(Head) + GetArgumentsEncodedSize(Tail...);
	}

	template <typename T>
	static typename TEnableIf<TAnd<TNot<TIsFloatingPoint<T>>, TNot<TIsStringArgument<T>>>::Value>::Type EncodeArgumentInternal(uint8*& TypeCodesPtr, uint8*& PayloadPtr, T Argument)
	{
		*TypeCodesPtr++ = FormatArgTypeCode_CategoryInteger | sizeof(T);

#if PLATFORM_SUPPORTS_UNALIGNED_LOADS
		*reinterpret_cast<T*>(PayloadPtr) = Argument;
#else
		// For ARM targets, it's possible that using __packed here would be preferable
		// but I have not checked the codegen -- it's possible that the compiler generates
		// the same code for this fixed size memcpy
		memcpy(PayloadPtr, &Argument, sizeof Argument);
#endif

		PayloadPtr += sizeof(T);
	}

	template <typename T>
	static typename TEnableIf<TIsFloatingPoint<T>::Value>::Type EncodeArgumentInternal(uint8*& TypeCodesPtr, uint8*& PayloadPtr, T Argument)
	{
		*TypeCodesPtr++ = FormatArgTypeCode_CategoryFloatingPoint | sizeof(T);

#if PLATFORM_SUPPORTS_UNALIGNED_LOADS
		*reinterpret_cast<T*>(PayloadPtr) = Argument;
#else
		// For ARM targets, it's possible that using __packed here would be preferable
		// but I have not checked the codegen -- it's possible that the compiler generates
		// the same code for this fixed size memcpy
		memcpy(PayloadPtr, &Argument, sizeof Argument);
#endif

		PayloadPtr += sizeof(T);
	}

	template <typename T, typename CharType = typename TRemoveCV<typename TRemovePointer<T>::Type>::Type>
	static typename TEnableIf<TIsStringArgument<T>::Value>::Type EncodeArgumentInternal(uint8*& TypeCodesPtr, uint8*& PayloadPtr, T Argument)
	{
		*TypeCodesPtr++ = FormatArgTypeCode_CategoryString | sizeof(CharType);
		if (Argument != nullptr)
		{
			uint16 Length = (uint16)((TCString<CharType>::Strlen(Argument) + 1) * sizeof(CharType));
			memcpy(PayloadPtr, Argument, Length);
			PayloadPtr += Length;
		}
		else
		{
			CharType Terminator { 0 };
			memcpy(PayloadPtr, &Terminator, sizeof(CharType));
			PayloadPtr += sizeof(CharType);
		}
	}

	constexpr static void EncodeArgumentsInternal(uint8*& TypeCodesPtr, uint8*& PayloadPtr)
	{
	}

	template <typename T, typename... Types>
	static void EncodeArgumentsInternal(uint8*& ArgDescriptorsPtr, uint8*& ArgPayloadPtr, T Head, Types... Tail)
	{
		EncodeArgumentInternal(ArgDescriptorsPtr, ArgPayloadPtr, Head);
		EncodeArgumentsInternal(ArgDescriptorsPtr, ArgPayloadPtr, Tail...);
	}
};