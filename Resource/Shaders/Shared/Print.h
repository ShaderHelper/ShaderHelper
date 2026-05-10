//Thanks to https://therealmjp.github.io/posts/hlsl-printf/

//However the string literal trick was already fixed, and CharToUint can not be used in spirv codegen.
//https://github.com/microsoft/DirectXShaderCompiler/pull/6920
//https://godbolt.org/z/bnz9TPME8
//Additionally, within a function template, uint cannot be initialized from a char rvalue.
//https://godbolt.org/z/G7MW9jP9z
//so reference ue's approach by preprocessing shaders to generate uint arrays.
#pragma once

#ifndef GPrivate_ENABLE_PRINT
    #define GPrivate_ENABLE_PRINT 1
#endif

#ifndef GPrivate_ENABLE_ASSERT
    #define GPrivate_ENABLE_ASSERT 1
#endif

#ifdef __cplusplus
#include "TypeAlias.h"

namespace HLSL
{
#endif

    enum GPrivate_TypeTag
    {
        GPrivate_Print_uint,
        GPrivate_Print_uint2,
        GPrivate_Print_uint3,
        GPrivate_Print_uint4,

        GPrivate_Print_int,
        GPrivate_Print_int2,
        GPrivate_Print_int3,
        GPrivate_Print_int4,

        GPrivate_Print_float,
        GPrivate_Print_float2,
        GPrivate_Print_float3,
        GPrivate_Print_float4,

        GPrivate_Print_bool,
        GPrivate_Print_bool2,
        GPrivate_Print_bool3,
        GPrivate_Print_bool4,

        GPrivate_Num,
    };

    struct GPrivate_PrinterBuffer
    {
        static const uint MaxBufferSize = 511;
        static const uint MaxBufferByteSize = MaxBufferSize * 4;

        uint ByteSize;
        //PrintBuffer layout: AssertFlag Line   [xxx{0}xxxx\0] [ArgNum TypeTag ArgValue ...]
        //Bytes:              ^1         ^2     ^CharNum       ^1     ^1      ^sizeof(ArgValue)
        uint PrintBuffer[MaxBufferSize];
    };

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
#include "Common.hlsl"

RWByteAddressBuffer GPrivate_Printer : register(u114, space0);

// Layout in RWByteAddressBuffer GPrivate_Printer:
// [0, 4)   : ByteSize (uint)
// [4, ...) : PrintBuffer bytes (GPrivate_PrinterBuffer::MaxBufferSize * 4 bytes)
// 1 byte append
uint GPrivate_AppendChar(uint Offset, uint Char)
{
    uint Shift       = (Offset % 4) * 8;
    uint BufferIndex = Offset / 4;
    uint ByteAddress = 4 + BufferIndex * 4;
    uint Original;
    GPrivate_Printer.InterlockedOr(ByteAddress, (Char & 0xFF) << Shift, Original);
    return Offset + 1;
}

template<typename T>
uint GPrivate_GetArgValue(T Arg)
{
    return asuint(Arg);
}

template<>
uint GPrivate_GetArgValue(bool Arg)
{
    return Arg ? 1 : 0;
}

template<typename T, int N>
uint GPrivate_AppendArg(uint Offset, GPrivate_TypeTag Tag, T Arg[N])
{
    uint NewOffset = GPrivate_AppendChar(Offset, Tag);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            uint ArgVal = GPrivate_GetArgValue(Arg[i]);
            uint Val = ArgVal >> (j * 8);
            NewOffset = GPrivate_AppendChar(NewOffset, Val);
        }
    }
    return NewOffset;
}

#define GPrivate_AppendArgFunc(Type)                 \
    uint GPrivate_AppendArg(uint Offset, Type Arg)                \
    {                                       \
        Type Arr[] = { Arg };         \
        return GPrivate_AppendArg(Offset, GPrivate_Print_##Type, Arr);         \
    }                                       \
    uint GPrivate_AppendArg(uint Offset, Type##2 Arg)                \
    {                                       \
        Type Arr[] = { Arg.x, Arg.y };    \
        return GPrivate_AppendArg(Offset, GPrivate_Print_##Type##2, Arr);                     \
    }                                       \
    uint GPrivate_AppendArg(uint Offset, Type##3 Arg)                \
    {                                       \
        Type Arr[] = { Arg.x, Arg.y, Arg.z }; \
        return GPrivate_AppendArg(Offset, GPrivate_Print_##Type##3, Arr);                                 \
    }                                       \
    uint GPrivate_AppendArg(uint Offset, Type##4 Arg)                \
    {                                       \
        Type Arr[] = { Arg.x, Arg.y, Arg.z, Arg.w };  \
        return GPrivate_AppendArg(Offset, GPrivate_Print_##Type##4, Arr);         \
    }

GPrivate_AppendArgFunc(uint)
GPrivate_AppendArgFunc(int)
GPrivate_AppendArgFunc(float)
GPrivate_AppendArgFunc(bool)

#define GPrivate_SELECT_NUM(Arg0, Arg1, Arg2, Arg3, RESULT, ...) RESULT
#define GPrivate_GET_ARG_NUM(...) GPrivate_SELECT_NUM(0, ##__VA_ARGS__, 3, 2, 1, 0)

#define GPrivate_GET_ARGS_SIZE_0() 0
#define GPrivate_GET_ARGS_SIZE_1(Arg1) sizeof(Arg1)
#define GPrivate_GET_ARGS_SIZE_2(Arg1, Arg2) sizeof(Arg1) + GPrivate_GET_ARGS_SIZE_1(Arg2)
#define GPrivate_GET_ARGS_SIZE_3(Arg1, Arg2, Arg3) sizeof(Arg1) + GPrivate_GET_ARGS_SIZE_2(Arg2, Arg3)
#define GPrivate_GET_ARGS_SIZE(...) JOIN(GPrivate_GET_ARGS_SIZE_, GPrivate_GET_ARG_NUM(__VA_ARGS__))(__VA_ARGS__)

#define GPrivate_APPEND_ARGS_0()
#define GPrivate_APPEND_ARGS_1(Arg1) ByteOffset = GPrivate_AppendArg(ByteOffset, Arg1)
#define GPrivate_APPEND_ARGS_2(Arg1, Arg2) ByteOffset = GPrivate_AppendArg(ByteOffset, Arg1); GPrivate_APPEND_ARGS_1(Arg2)
#define GPrivate_APPEND_ARGS_3(Arg1, Arg2, Arg3) ByteOffset = GPrivate_AppendArg(ByteOffset, Arg1); GPrivate_APPEND_ARGS_2(Arg2, Arg3)
#define GPrivate_APPEND_ARGS(...) JOIN(GPrivate_APPEND_ARGS_, GPrivate_GET_ARG_NUM(__VA_ARGS__))(__VA_ARGS__)

#define GPrivate_UNUSED_ARGS_0()
#define GPrivate_UNUSED_ARGS_1(Arg1) (void)Arg1
#define GPrivate_UNUSED_ARGS_2(Arg1, Arg2) (void)Arg1; GPrivate_UNUSED_ARGS_1(Arg2);
#define GPrivate_UNUSED_ARGS_3(Arg1, Arg2, Arg3) (void)Arg1; GPrivate_UNUSED_ARGS_2(Arg2, Arg3);
#define GPrivate_UNUSED_ARGS(...) JOIN(GPrivate_UNUSED_ARGS_, GPrivate_GET_ARG_NUM(__VA_ARGS__))(__VA_ARGS__)

#if GPrivate_ENABLE_PRINT == 1
//Up to 3 args now.
//Print("abc {0}", t);
#define Print(StrArrDecl, ...)  do {                                    \
    uint StrArr[] = {0};                                                \
    {                                                                   \
        StrArrDecl;                                                     \
        uint CharNum = sizeof(StrArr) / sizeof(StrArr[0]);              \
        uint ArgNum = GPrivate_GET_ARG_NUM(__VA_ARGS__);                \
        uint ArgByteSize = 1 + ArgNum + GPrivate_GET_ARGS_SIZE(__VA_ARGS__); \
        uint Increment = 3 + CharNum + ArgByteSize;                     \
        uint OldByteSize = GPrivate_Printer.Load(0);                    \
        uint ByteOffset = GPrivate_PrinterBuffer::MaxBufferByteSize;    \
        while(OldByteSize + Increment <= GPrivate_PrinterBuffer::MaxBufferByteSize) \
        {                                                               \
            uint CompareValue = OldByteSize;                            \
            GPrivate_Printer.InterlockedCompareExchange(0,              \
                CompareValue, OldByteSize + Increment, OldByteSize);    \
            if(OldByteSize == CompareValue)                             \
            {                                                           \
                ByteOffset = OldByteSize;                               \
                break;                                                  \
            }                                                           \
        }                                                               \
        if (ByteOffset >= GPrivate_PrinterBuffer::MaxBufferByteSize)    \
        {                                                               \
            break;                                                      \
        }                                                               \
        ByteOffset = GPrivate_AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, __LINE__ & 0xFF);  \
        ByteOffset = GPrivate_AppendChar(ByteOffset, (__LINE__ >> 8) & 0xFF); \
        for (uint i = 0; i < CharNum; i++)                              \
        {                                                               \
            ByteOffset = GPrivate_AppendChar(ByteOffset, StrArr[i]);    \
        }                                                               \
        ByteOffset = GPrivate_AppendChar(ByteOffset, ArgNum);           \
        GPrivate_APPEND_ARGS(__VA_ARGS__);                              \
    }                                                                   \
} while(0)
#elif GPrivate_EDITOR_ISENSE == 1
#define Print(Str, ...) GPrivate_UNUSED_ARGS(__VA_ARGS__);
#else
#define Print(Str, ...)
#endif

static uint GPrivate_AssertResult = 1;

#if GPrivate_ENABLE_ASSERT == 1
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
#else
#define Assert(...)
#define AssertFormat(...)
#endif

#endif
