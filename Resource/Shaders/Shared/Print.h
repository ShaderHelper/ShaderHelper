//Thanks to https://therealmjp.github.io/posts/hlsl-printf/

//However the string literal trick was already fixed, 
//so reference ue's approach by preprocessing shaders to generate uint arrays.
//https://github.com/microsoft/DirectXShaderCompiler/pull/6920
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

		Num,
	};

	struct Printer
	{
		static const uint MaxBufferSize = 256;

		uint ByteSize;
		//PrintBuffer layout: [xxx{0}xxxx\0] [ArgNum TypeTag ArgValue ...]
		//Bytes:               ^CharNum        ^1     ^1      ^sizeof(ArgValue)
		uint PrintBuffer[MaxBufferSize];
	};

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

DECLARE_GLOBAL_RW_BUFFER(RWStructuredBuffer<Printer>, Printer, 0)

//1byte
uint AppendChar(uint Offset, uint Char)
{
	uint Shift = (Offset % 4) * 8;
	uint BufferIndex = Offset / 4;
	InterlockedOr(Printer[0].PrintBuffer[BufferIndex], (Char & 0xFF) << Shift);
	return Offset + 1;
}

template<int N>
uint AppendArg(uint Offset, TypeTag Tag, uint Arg[N])
{
	uint NewOffset = AppendChar(Offset, Tag);

	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			uint Val = Arg[i] >> (j * 8);
			NewOffset = AppendChar(NewOffset, Val);
		}
	}
	return NewOffset;
}

#define AppendArgFunc(Type)                 \
    uint AppendArg(uint Offset, Type Arg)                \
    {                                       \
        uint Arr[] = { asuint(Arg) };         \
        return AppendArg(Offset, Print_##Type, Arr);         \
    }                                       \
    uint AppendArg(uint Offset, Type##2 Arg)                \
    {                                       \
        uint Arr[] = { asuint(Arg.x), asuint(Arg.y) };    \
        return AppendArg(Offset, Print_##Type##2, Arr);                     \
    }                                       \
    uint AppendArg(uint Offset, Type##3 Arg)                \
    {                                       \
        uint Arr[] = { asuint(Arg.x), asuint(Arg.y), asuint(Arg.z) }; \
        return AppendArg(Offset, Print_##Type##3, Arr);                                 \
    }                                       \
    uint AppendArg(uint Offset, Type##4 Arg)                \
    {                                       \
        uint Arr[] = { asuint(Arg.x), asuint(Arg.y), asuint(Arg.z), asuint(Arg.w) };  \
        return AppendArg(Offset, Print_##Type##4, Arr);         \
    }

AppendArgFunc(uint)
AppendArgFunc(int)
AppendArgFunc(float)

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

//Up to 3 args now.
//Print("abc {0}", t);
#define Print(StrArrDecl, ...)  do {                                \
    StrArrDecl;                                                     \
    uint CharNum = sizeof(StrArr) / sizeof(StrArr[0]);              \
    uint ArgNum = GET_ARG_NUM(__VA_ARGS__);                         \
	uint ArgByteSize = 1 + ArgNum + GET_ARGS_SIZE(__VA_ARGS__);     \
    uint Increment = CharNum + ArgByteSize;                         \
    uint OldByteSize = Printer[0].ByteSize;                         \
	uint ByteOffset = 0xFFFFFFFF;                                   \
    [allow_uav_condition]                                           \
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
	for (uint i = 0; i < CharNum; i++)                              \
	{                                                               \
		ByteOffset = AppendChar(ByteOffset, StrArr[i]);             \
	}                                                               \
	ByteOffset = AppendChar(ByteOffset, ArgNum);                    \
	APPEND_ARGS(__VA_ARGS__);                                       \
} while(0)

static uint GAssertCondition = 1;
void Assert(uint Condition) 
{
	GAssertCondition &= Condition;
}

#endif
