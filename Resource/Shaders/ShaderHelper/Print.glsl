#ifndef GPrivate_PRINT_GLSL_INCLUDED
#define GPrivate_PRINT_GLSL_INCLUDED

#ifndef GPrivate_ENABLE_PRINT
    #define GPrivate_ENABLE_PRINT 1
#endif

#ifndef ENABLE_ASSERT
    #define ENABLE_ASSERT 1
#endif

#extension GL_EXT_debug_printf : enable

const uint GPrivate_PRINT_MAX_U32 = 511u;
const uint GPrivate_PRINT_MAX_BYTES = GPrivate_PRINT_MAX_U32 * 4u;

layout(std430, binding = 114) buffer GPrivate_PrinterBuffer {
    uint Data[];
} GPrivate_Printer;

uint GPrivate_AppendChar(uint Offset, uint Char)
{
    uint Shift = (Offset % 4u) * 8u;
    uint BufferIndex = Offset / 4u;
    atomicOr(GPrivate_Printer.Data[1u + BufferIndex], (Char & 0xFFu) << Shift);
    return Offset + 1u;
}

uint GPrivate_GetArgValue(uint v) { return v; }
uint GPrivate_GetArgValue(int v) { return uint(v); }
uint GPrivate_GetArgValue(float v) { return floatBitsToUint(v); }
uint GPrivate_GetArgValue(bool v) { return v ? 1u : 0u; }

uint GPrivate_GetArgSize(uint) { return 4u; }
uint GPrivate_GetArgSize(uvec2) { return 8u; }
uint GPrivate_GetArgSize(uvec3) { return 12u; }
uint GPrivate_GetArgSize(uvec4) { return 16u; }
uint GPrivate_GetArgSize(int) { return 4u; }
uint GPrivate_GetArgSize(ivec2) { return 8u; }
uint GPrivate_GetArgSize(ivec3) { return 12u; }
uint GPrivate_GetArgSize(ivec4) { return 16u; }
uint GPrivate_GetArgSize(float) { return 4u; }
uint GPrivate_GetArgSize(vec2) { return 8u; }
uint GPrivate_GetArgSize(vec3) { return 12u; }
uint GPrivate_GetArgSize(vec4) { return 16u; }
uint GPrivate_GetArgSize(bool) { return 4u; }
uint GPrivate_GetArgSize(bvec2) { return 8u; }
uint GPrivate_GetArgSize(bvec3) { return 12u; }
uint GPrivate_GetArgSize(bvec4) { return 16u; }

uint GPrivate_AppendArg_Array(uint Offset, uint Tag, const uint vals[4], int count)
{
    uint NewOffset = GPrivate_AppendChar(Offset, Tag);
    for (int i = 0; i < count; ++i)
    {
        uint ArgVal = vals[i];
        for (int j = 0; j < 4; ++j)
        {
            uint Val = (ArgVal >> (j * 8)) & 0xFFu;
            NewOffset = GPrivate_AppendChar(NewOffset, Val);
        }
    }
    return NewOffset;
}

const uint GPrivate_Print_uint   = 0u;
const uint GPrivate_Print_uint2  = 1u;
const uint GPrivate_Print_uint3  = 2u;
const uint GPrivate_Print_uint4  = 3u;
const uint GPrivate_Print_int    = 4u;
const uint GPrivate_Print_int2   = 5u;
const uint GPrivate_Print_int3   = 6u;
const uint GPrivate_Print_int4   = 7u;
const uint GPrivate_Print_float  = 8u;
const uint GPrivate_Print_float2 = 9u;
const uint GPrivate_Print_float3 = 10u;
const uint GPrivate_Print_float4 = 11u;
const uint GPrivate_Print_bool   = 12u;
const uint GPrivate_Print_bool2  = 13u;
const uint GPrivate_Print_bool3  = 14u;
const uint GPrivate_Print_bool4  = 15u;

uint GPrivate_AppendArg(uint Offset, uint v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_uint, arr, 1); }
uint GPrivate_AppendArg(uint Offset, uvec2 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_uint2, arr, 2); }
uint GPrivate_AppendArg(uint Offset, uvec3 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_uint3, arr, 3); }
uint GPrivate_AppendArg(uint Offset, uvec4 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); arr[3] = GPrivate_GetArgValue(v.w); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_uint4, arr, 4); }

uint GPrivate_AppendArg(uint Offset, int v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_int, arr, 1); }
uint GPrivate_AppendArg(uint Offset, ivec2 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_int2, arr, 2); }
uint GPrivate_AppendArg(uint Offset, ivec3 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_int3, arr, 3); }
uint GPrivate_AppendArg(uint Offset, ivec4 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); arr[3] = GPrivate_GetArgValue(v.w); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_int4, arr, 4); }

uint GPrivate_AppendArg(uint Offset, float v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_float, arr, 1); }
uint GPrivate_AppendArg(uint Offset, vec2 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_float2, arr, 2); }
uint GPrivate_AppendArg(uint Offset, vec3 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_float3, arr, 3); }
uint GPrivate_AppendArg(uint Offset, vec4 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); arr[3] = GPrivate_GetArgValue(v.w); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_float4, arr, 4); }

uint GPrivate_AppendArg(uint Offset, bool v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_bool, arr, 1); }
uint GPrivate_AppendArg(uint Offset, bvec2 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_bool2, arr, 2); }
uint GPrivate_AppendArg(uint Offset, bvec3 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_bool3, arr, 3); }
uint GPrivate_AppendArg(uint Offset, bvec4 v) { uint arr[4]; arr[0] = GPrivate_GetArgValue(v.x); arr[1] = GPrivate_GetArgValue(v.y); arr[2] = GPrivate_GetArgValue(v.z); arr[3] = GPrivate_GetArgValue(v.w); return GPrivate_AppendArg_Array(Offset, GPrivate_Print_bool4, arr, 4); }

uint GPrivate_AssertResult = 1u;

#define EXPAND(a) a

#if GPrivate_ENABLE_PRINT == 1
#define Print0(StrArrDecl) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint Increment = 3u + CharNum + 1u; \
    uint OldByteSize = GPrivate_Printer.Data[0]; \
    uint ByteOffset = GPrivate_PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= GPrivate_PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(GPrivate_Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < GPrivate_PRINT_MAX_BYTES) { \
        ByteOffset = GPrivate_AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = GPrivate_AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, 0u); \
    } \
}

#define Print1(StrArrDecl, Arg1) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint ArgByteSize = 1u + 1u + GPrivate_GetArgSize(Arg1); \
    uint Increment = 3u + CharNum + ArgByteSize; \
    uint OldByteSize = GPrivate_Printer.Data[0]; \
    uint ByteOffset = GPrivate_PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= GPrivate_PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(GPrivate_Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < GPrivate_PRINT_MAX_BYTES) { \
        ByteOffset = GPrivate_AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = GPrivate_AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, 1u); \
        ByteOffset = GPrivate_AppendArg(ByteOffset, Arg1); \
    } \
}

#define Print2(StrArrDecl, Arg1, Arg2) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint ArgByteSize = 1u + 2u + GPrivate_GetArgSize(Arg1) + GPrivate_GetArgSize(Arg2); \
    uint Increment = 3u + CharNum + ArgByteSize; \
    uint OldByteSize = GPrivate_Printer.Data[0]; \
    uint ByteOffset = GPrivate_PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= GPrivate_PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(GPrivate_Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < GPrivate_PRINT_MAX_BYTES) { \
        ByteOffset = GPrivate_AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = GPrivate_AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, 2u); \
        ByteOffset = GPrivate_AppendArg(ByteOffset, Arg1); \
        ByteOffset = GPrivate_AppendArg(ByteOffset, Arg2); \
    } \
}

#define Print3(StrArrDecl, Arg1, Arg2, Arg3) { \
    StrArrDecl; \
    uint CharNum = uint(StrArr.length()); \
    uint ArgByteSize = 1u + 3u + GPrivate_GetArgSize(Arg1) + GPrivate_GetArgSize(Arg2) + GPrivate_GetArgSize(Arg3); \
    uint Increment = 3u + CharNum + ArgByteSize; \
    uint OldByteSize = GPrivate_Printer.Data[0]; \
    uint ByteOffset = GPrivate_PRINT_MAX_BYTES; \
    while (OldByteSize + Increment <= GPrivate_PRINT_MAX_BYTES) { \
        uint Prev = atomicCompSwap(GPrivate_Printer.Data[0], OldByteSize, OldByteSize + Increment); \
        if (Prev == OldByteSize) { ByteOffset = OldByteSize; break; } \
        OldByteSize = Prev; \
    } \
    if (ByteOffset < GPrivate_PRINT_MAX_BYTES) { \
        ByteOffset = GPrivate_AppendChar(ByteOffset, GPrivate_AssertResult); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, uint(__LINE__) & 0xFFu); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, (uint(__LINE__) >> 8u) & 0xFFu); \
        for (uint i = 0u; i < CharNum; ++i) ByteOffset = GPrivate_AppendChar(ByteOffset, StrArr[i]); \
        ByteOffset = GPrivate_AppendChar(ByteOffset, 3u); \
        ByteOffset = GPrivate_AppendArg(ByteOffset, Arg1); \
        ByteOffset = GPrivate_AppendArg(ByteOffset, Arg2); \
        ByteOffset = GPrivate_AppendArg(ByteOffset, Arg3); \
    } \
}
#elif GPrivate_EDITOR_ISENSE == 1    
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
#elif GPrivate_EDITOR_ISENSE == 1
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

#endif
