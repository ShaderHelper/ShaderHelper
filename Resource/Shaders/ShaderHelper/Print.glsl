#ifndef ENABLE_PRINT
    #define ENABLE_PRINT 1
#endif

#ifndef ENABLE_ASSERT
    #define ENABLE_ASSERT 1
#endif

#extension GL_EXT_debug_printf : enable

const uint PRINT_MAX_U32 = 511u;
const uint PRINT_MAX_BYTES = PRINT_MAX_U32 * 4u;

layout(std430, binding = 0) buffer PrinterBuffer {
    uint Data[];
} Printer;

uint AppendChar(uint Offset, uint Char)
{
    uint Shift = (Offset % 4u) * 8u;
    uint BufferIndex = Offset / 4u;
    atomicOr(Printer.Data[1u + BufferIndex], (Char & 0xFFu) << Shift);
    return Offset + 1u;
}

uint GetArgValue(uint v) { return v; }
uint GetArgValue(int v) { return uint(v); }
uint GetArgValue(float v) { return floatBitsToUint(v); }
uint GetArgValue(bool v) { return v ? 1u : 0u; }

uint GetArgSize(uint) { return 4u; }
uint GetArgSize(uvec2) { return 8u; }
uint GetArgSize(uvec3) { return 12u; }
uint GetArgSize(uvec4) { return 16u; }
uint GetArgSize(int) { return 4u; }
uint GetArgSize(ivec2) { return 8u; }
uint GetArgSize(ivec3) { return 12u; }
uint GetArgSize(ivec4) { return 16u; }
uint GetArgSize(float) { return 4u; }
uint GetArgSize(vec2) { return 8u; }
uint GetArgSize(vec3) { return 12u; }
uint GetArgSize(vec4) { return 16u; }
uint GetArgSize(bool) { return 4u; }
uint GetArgSize(bvec2) { return 8u; }
uint GetArgSize(bvec3) { return 12u; }
uint GetArgSize(bvec4) { return 16u; }

uint AppendArg_Array(uint Offset, uint Tag, const uint vals[4], int count)
{
    uint NewOffset = AppendChar(Offset, Tag);
    for (int i = 0; i < count; ++i)
    {
        uint ArgVal = vals[i];
        for (int j = 0; j < 4; ++j)
        {
            uint Val = (ArgVal >> (j * 8)) & 0xFFu;
            NewOffset = AppendChar(NewOffset, Val);
        }
    }
    return NewOffset;
}

const uint Print_uint   = 0u;
const uint Print_uint2  = 1u;
const uint Print_uint3  = 2u;
const uint Print_uint4  = 3u;
const uint Print_int    = 4u;
const uint Print_int2   = 5u;
const uint Print_int3   = 6u;
const uint Print_int4   = 7u;
const uint Print_float  = 8u;
const uint Print_float2 = 9u;
const uint Print_float3 = 10u;
const uint Print_float4 = 11u;
const uint Print_bool   = 12u;
const uint Print_bool2  = 13u;
const uint Print_bool3  = 14u;
const uint Print_bool4  = 15u;

uint AppendArg(uint Offset, uint v) { uint arr[4]; arr[0] = GetArgValue(v); return AppendArg_Array(Offset, Print_uint, arr, 1); }
uint AppendArg(uint Offset, uvec2 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); return AppendArg_Array(Offset, Print_uint2, arr, 2); }
uint AppendArg(uint Offset, uvec3 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); return AppendArg_Array(Offset, Print_uint3, arr, 3); }
uint AppendArg(uint Offset, uvec4 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); arr[3] = GetArgValue(v.w); return AppendArg_Array(Offset, Print_uint4, arr, 4); }

uint AppendArg(uint Offset, int v) { uint arr[4]; arr[0] = GetArgValue(v); return AppendArg_Array(Offset, Print_int, arr, 1); }
uint AppendArg(uint Offset, ivec2 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); return AppendArg_Array(Offset, Print_int2, arr, 2); }
uint AppendArg(uint Offset, ivec3 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); return AppendArg_Array(Offset, Print_int3, arr, 3); }
uint AppendArg(uint Offset, ivec4 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); arr[3] = GetArgValue(v.w); return AppendArg_Array(Offset, Print_int4, arr, 4); }

uint AppendArg(uint Offset, float v) { uint arr[4]; arr[0] = GetArgValue(v); return AppendArg_Array(Offset, Print_float, arr, 1); }
uint AppendArg(uint Offset, vec2 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); return AppendArg_Array(Offset, Print_float2, arr, 2); }
uint AppendArg(uint Offset, vec3 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); return AppendArg_Array(Offset, Print_float3, arr, 3); }
uint AppendArg(uint Offset, vec4 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); arr[3] = GetArgValue(v.w); return AppendArg_Array(Offset, Print_float4, arr, 4); }

uint AppendArg(uint Offset, bool v) { uint arr[4]; arr[0] = GetArgValue(v); return AppendArg_Array(Offset, Print_bool, arr, 1); }
uint AppendArg(uint Offset, bvec2 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); return AppendArg_Array(Offset, Print_bool2, arr, 2); }
uint AppendArg(uint Offset, bvec3 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); return AppendArg_Array(Offset, Print_bool3, arr, 3); }
uint AppendArg(uint Offset, bvec4 v) { uint arr[4]; arr[0] = GetArgValue(v.x); arr[1] = GetArgValue(v.y); arr[2] = GetArgValue(v.z); arr[3] = GetArgValue(v.w); return AppendArg_Array(Offset, Print_bool4, arr, 4); }

uint GPrivate_AssertResult = 1u;

#define EXPAND(a) a

#if ENABLE_PRINT == 1
#define Print0(StrArrDecl) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint Increment = 3u + CharNum + 1u; \
    uint OldByteSize = Printer.Data[0]; \
    uint ByteOffset = PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < PRINT_MAX_BYTES) { \
        ByteOffset = AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = AppendChar(ByteOffset, 0u); \
    } \
}

#define Print1(StrArrDecl, Arg1) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint ArgByteSize = 1u + GetArgSize(Arg1); \
    uint Increment = 3u + CharNum + ArgByteSize; \
    uint OldByteSize = Printer.Data[0]; \
    uint ByteOffset = PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < PRINT_MAX_BYTES) { \
        ByteOffset = AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = AppendChar(ByteOffset, 1u); \
        ByteOffset = AppendArg(ByteOffset, Arg1); \
    } \
}

#define Print2(StrArrDecl, Arg1, Arg2) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint ArgByteSize = 1u + GetArgSize(Arg1) + GetArgSize(Arg2); \
    uint Increment = 3u + CharNum + ArgByteSize; \
    uint OldByteSize = Printer.Data[0]; \
    uint ByteOffset = PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < PRINT_MAX_BYTES) { \
        ByteOffset = AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = AppendChar(ByteOffset, 2u); \
        ByteOffset = AppendArg(ByteOffset, Arg1); \
        ByteOffset = AppendArg(ByteOffset, Arg2); \
    } \
}

#define Print3(StrArrDecl, Arg1, Arg2, Arg3) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint ArgByteSize = 1u + GetArgSize(Arg1) + GetArgSize(Arg2) + GetArgSize(Arg3); \
    uint Increment = 3u + CharNum + ArgByteSize; \
    uint OldByteSize = Printer.Data[0]; \
    uint ByteOffset = PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < PRINT_MAX_BYTES) { \
        ByteOffset = AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = AppendChar(ByteOffset, 3u); \
        ByteOffset = AppendArg(ByteOffset, Arg1); \
        ByteOffset = AppendArg(ByteOffset, Arg2); \
        ByteOffset = AppendArg(ByteOffset, Arg3); \
    } \
}
#elif EDITOR_ISENSE == 1    
#define Print0(StrArrDecl) StrArrDecl;
#define Print1(StrArrDecl, Arg1) StrArrDecl;Arg1;
#define Print2(StrArrDecl, Arg1, Arg2) StrArrDecl;Arg1;Arg2;
#define Print3(StrArrDecl, Arg1, Arg2, Arg3) StrArrDecl;Arg1;Arg2;Arg3;
#else
#define Print0(StrArrDecl)
#define Print1(StrArrDecl, Arg1)
#define Print2(StrArrDecl, Arg1, Arg2)
#define Print3(StrArrDecl, Arg1, Arg2, Arg3)
#endif

#if ENABLE_ASSERT == 1
#define Assert0(Cond) { \
    GPrivate_AssertResult &= (Cond) ? 1u : 0u; \
    if (GPrivate_AssertResult != 1u) { \
        Print0(EXPAND(uint StrArr[] = uint[](0u))); \
    } \
}

#define Assert1(Cond, StrArrDecl) { \
    GPrivate_AssertResult &= (Cond) ? 1u : 0u; \
    if (GPrivate_AssertResult != 1u) { \
        Print0(StrArrDecl); \
    } \
}

#define AssertFormat1(Cond, StrArrDecl, Arg1) { \
    GPrivate_AssertResult &= (Cond) ? 1u : 0u; \
    if (GPrivate_AssertResult != 1u) { \
        Print1(StrArrDecl, Arg1); \
    } \
}

#define AssertFormat2(Cond, StrArrDecl, Arg1, Arg2) { \
    GPrivate_AssertResult &= (Cond) ? 1u : 0u; \
    if (GPrivate_AssertResult != 1u) { \
        Print2(StrArrDecl, Arg1, Arg2); \
    } \
}

#define AssertFormat3(Cond, StrArrDecl, Arg1, Arg2, Arg3) { \
    GPrivate_AssertResult &= (Cond) ? 1u : 0u; \
    if (GPrivate_AssertResult != 1u) { \
        Print3(StrArrDecl, Arg1, Arg2, Arg3); \
    } \
}
#elif EDITOR_ISENSE == 1
#define Assert0(Cond) Cond;
#define Assert1(Cond, StrArrDecl) Cond;StrArrDecl;
#define AssertFormat1(Cond, StrArrDecl, Arg1) Cond;StrArrDecl;Arg1;
#define AssertFormat2(Cond, StrArrDecl, Arg1, Arg2) Cond;StrArrDecl;Arg1;Arg2;
#define AssertFormat3(Cond, StrArrDecl, Arg1, Arg2, Arg3) Cond;StrArrDecl;Arg1;Arg2;Arg3;
#else    
#define Assert0(Cond)
#define Assert1(Cond, StrArrDecl)
#define AssertFormat1(Cond, StrArrDecl, Arg1)
#define AssertFormat2(Cond, StrArrDecl, Arg1, Arg2)
#define AssertFormat3(Cond, StrArrDecl, Arg1, Arg2, Arg3)
#endif
