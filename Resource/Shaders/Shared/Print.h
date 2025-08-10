//Thanks to https://therealmjp.github.io/posts/hlsl-printf/

//However the string literal trick was already fixed, and CharToUint can not be used in spirv codegen.
//so reference ue's approach by preprocessing shaders to generate uint arrays.
//https://github.com/microsoft/DirectXShaderCompiler/pull/6920
//https://godbolt.org/z/bnz9TPME8
#pragma once

#ifdef __cplusplus
#include "TypeAlias.h"

namespace HLSL
{
#endif

	enum TypeTag
	{
		Print_uint,
		Print_uint2,
		Print_uint3,
		Print_uint4,

		Print_int,
		Print_int2,
		Print_int3,
		Print_int4,

		Print_float,
		Print_float2,
		Print_float3,
		Print_float4,

		Print_bool,
		Print_bool2,
		Print_bool3,
		Print_bool4,

		Num,
	};

	struct Printer
	{
		static const uint MaxBufferSize = 511;

		uint ByteSize;
		//PrintBuffer layout: AssertFlag Line [xxx{0}xxxx\0] [ArgNum TypeTag ArgValue ...]
		//Bytes:              ^1         ^1    ^CharNum       ^1     ^1      ^sizeof(ArgValue)
		uint PrintBuffer[MaxBufferSize];
	};

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
#include "Common.hlsl"

DECLARE_GLOBAL_RW_BUFFER(RWStructuredBuffer<Printer>, Printer, 0)

//1byte
uint AppendChar(uint Offset, uint Char)
{
	uint Shift = (Offset % 4) * 8;
	uint BufferIndex = Offset / 4;
	InterlockedOr(Printer[0].PrintBuffer[BufferIndex], (Char & 0xFF) << Shift);
	return Offset + 1;
}

template<typename T>
uint GetArgValue(T Arg)
{
	return asuint(Arg);
}

template<>
uint GetArgValue(bool Arg)
{
	return Arg ? 1 : 0;
}

template<typename T, int N>
uint AppendArg(uint Offset, TypeTag Tag, T Arg[N])
{
	uint NewOffset = AppendChar(Offset, Tag);

	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			uint ArgVal = GetArgValue(Arg[i]);
			uint Val = ArgVal >> (j * 8);
			NewOffset = AppendChar(NewOffset, Val);
		}
	}
	return NewOffset;
}

#define AppendArgFunc(Type)                 \
    uint AppendArg(uint Offset, Type Arg)                \
    {                                       \
        Type Arr[] = { Arg };         \
        return AppendArg(Offset, Print_##Type, Arr);         \
    }                                       \
    uint AppendArg(uint Offset, Type##2 Arg)                \
    {                                       \
        Type Arr[] = { Arg.x, Arg.y };    \
        return AppendArg(Offset, Print_##Type##2, Arr);                     \
    }                                       \
    uint AppendArg(uint Offset, Type##3 Arg)                \
    {                                       \
        Type Arr[] = { Arg.x, Arg.y, Arg.z }; \
        return AppendArg(Offset, Print_##Type##3, Arr);                                 \
    }                                       \
    uint AppendArg(uint Offset, Type##4 Arg)                \
    {                                       \
        Type Arr[] = { Arg.x, Arg.y, Arg.z, Arg.w };  \
        return AppendArg(Offset, Print_##Type##4, Arr);         \
    }

AppendArgFunc(uint)
AppendArgFunc(int)
AppendArgFunc(float)
AppendArgFunc(bool)

#define SELECT_NUM(Arg0, Arg1, Arg2, Arg3, RESULT, ...) RESULT
#define GET_ARG_NUM(...) SELECT_NUM(0, ##__VA_ARGS__, 3, 2, 1, 0)

#define GET_ARGS_SIZE_0() 0
#define GET_ARGS_SIZE_1(Arg1) sizeof(Arg1)
#define GET_ARGS_SIZE_2(Arg1, Arg2) sizeof(Arg1) + GET_ARGS_SIZE_1(Arg2)
#define GET_ARGS_SIZE_3(Arg1, Arg2, Arg3) sizeof(Arg1) + GET_ARGS_SIZE_2(Arg2, Arg3)
#define GET_ARGS_SIZE(...) JOIN(GET_ARGS_SIZE_, GET_ARG_NUM(__VA_ARGS__))(__VA_ARGS__)

#define APPEND_ARGS_0()
#define APPEND_ARGS_1(Arg1) ByteOffset = AppendArg(ByteOffset, Arg1)
#define APPEND_ARGS_2(Arg1, Arg2) ByteOffset = AppendArg(ByteOffset, Arg1); APPEND_ARGS_1(Arg2)
#define APPEND_ARGS_3(Arg1, Arg2, Arg3) ByteOffset = AppendArg(ByteOffset, Arg1); APPEND_ARGS_2(Arg2, Arg3)
#define APPEND_ARGS(...) JOIN(APPEND_ARGS_, GET_ARG_NUM(__VA_ARGS__))(__VA_ARGS__)

#ifndef ENABLE_PRINT
	#define ENABLE_PRINT 1
#endif

#if ENABLE_PRINT == 1
//Up to 3 args now.
//Print("abc {0}", t);
#define Print(StrArrDecl, ...)  do {                                    \
	uint StrArr[] = {'\0'};                                             \
	{                                                                   \
		StrArrDecl;                                                     \
		uint CharNum = sizeof(StrArr) / sizeof(StrArr[0]);              \
		uint ArgNum = GET_ARG_NUM(__VA_ARGS__);                         \
		uint ArgByteSize = 1 + ArgNum + GET_ARGS_SIZE(__VA_ARGS__);     \
		uint Increment = 2 + CharNum + ArgByteSize;                     \
		uint OldByteSize = Printer[0].ByteSize;                         \
		uint ByteOffset = 0xFFFFFFFF;                                   \
		while(OldByteSize + Increment <= Printer::MaxBufferSize * 4)    \
		{                                                               \
			uint CompareValue = OldByteSize;                            \
			InterlockedCompareExchange(Printer[0].ByteSize,             \
				CompareValue, OldByteSize + Increment, OldByteSize);    \
			if(OldByteSize == CompareValue)                             \
			{                                                           \
				ByteOffset = OldByteSize;                               \
				break;                                                  \
			}                                                           \
		}                                                               \
		if (ByteOffset == 0xFFFFFFFF)                                   \
		{                                                               \
			break;                                                      \
		}                                                               \
		ByteOffset = AppendChar(ByteOffset, GPrivate_AssertResult);     \
		ByteOffset = AppendChar(ByteOffset, __LINE__);                  \
		for (uint i = 0; i < CharNum; i++)                              \
		{                                                               \
			ByteOffset = AppendChar(ByteOffset, StrArr[i]);             \
		}                                                               \
		ByteOffset = AppendChar(ByteOffset, ArgNum);                    \
		APPEND_ARGS(__VA_ARGS__);                                       \
	}                                                                   \
} while(0)
#else
#define Print(StrArrDecl, ...)
#endif

static uint GPrivate_AssertResult = 1;

//Assert(uv.x > 0.9);
//Assert(uv.x > 0.9, "uv.x must be greater than 0.9");
#define Assert(Condition, ...) do {                     \
	GPrivate_AssertResult &= Condition;                 \
	if(GPrivate_AssertResult != 1)                      \
	{                                                   \
		Print(EXPAND(__VA_ARGS__));                     \
	}                                                   \
}while(0)

//AssertFormat(uv.x > 0.9, "uv.x {0} must be greater than 0.9", uv.x);
#define AssertFormat(Condition, StrArrDecl, ...) do {   \
	GPrivate_AssertResult &= Condition;                 \
	if(GPrivate_AssertResult != 1)                      \
	{                                                   \
		Print(EXPAND(StrArrDecl), ##__VA_ARGS__);       \
	}                                                   \
}while(0)

#endif
