#define OpImageSampleImplicitLod 87
#define OpConvertFToU 109
#define OpConvertFToS 110
#define OpConvertSToF 111
#define OpConvertUToF 112
#define OpBitcast 124
#define OpFNegate 127
#define OpIAdd 128
#define OpFAdd 129
#define OpISub 130
#define OpFSub 131
#define OpIMul 132
#define OpFMul 133
#define OpUDiv 134
#define OpSDiv 135
#define OpFDiv 136
#define OpUMod 137
#define OpSRem 138
#define OpFRem 140
#define OpVectorTimesScalar 142
#define OpDot 148
#define OpAny 154
#define OpAll 155
#define OpIsNan 156
#define OpIsInf 157
#define OpLogicalOr 166
#define OpLogicalAnd 167
#define OpLogicalNot 168
#define OpSelect 169
#define OpIEqual 170
#define	OpINotEqual 171
#define	OpSGreaterThan 173
#define OpSLessThan 177
#define OpFOrdLessThan 184
#define OpFOrdGreaterThan 186
#define OpShiftRightLogical 194
#define OpShiftRightArithmetic 195
#define OpShiftLeftLogical 196
#define OpBitwiseOr 197
#define OpBitwiseXor 198
#define OpBitwiseAnd 199
#define OpNot 200
#define OpDPdx 207
#define OpDPdy 208
#define OpFwidth 209

#define RoundEven 2
#define Trunc 3
#define FAbs 4
#define SAbs 5
#define Floor 8
#define Ceil 9
#define Fract 10
#define Sin 13
#define Cos 14
#define Tan 15
#define Asin 16
#define Acos 17
#define Atan 18
#define Pow 26
#define Exp 27
#define Log 28
#define Exp2 29
#define Log2 30
#define Sqrt 31
#define InverseSqrt 32
#define UMin 38
#define SMin 39
#define UMax 41
#define SMax 42
#define FClamp 43
#define UClamp 44
#define SClamp 45
#define FMix 46
#define Step 48
#define SmoothStep 49
#define Length 66
#define Distance 67
#define Cross 68
#define Normalize 69
#define Reflect 71
#define NMin 79
#define NMax 80

#define MAP_TYPE_IMPL(A, B) A##B
#define MAP_TYPE(type) MAP_TYPE_IMPL(Type_, type)

#define Type_float 1001
#define Type_float2 1002
#define Type_float3 1003
#define Type_float4 1004
#define Type_int 1005
#define Type_int2 1006
#define Type_int3 1007
#define Type_int4 1008
#define Type_uint 1009
#define Type_uint2 1010
#define Type_uint3 1011
#define Type_uint4 1012

#define IsFloatResultType (MAP_TYPE(OpResultType) == Type_float || MAP_TYPE(OpResultType) == Type_float2 || MAP_TYPE(OpResultType) == Type_float3 || MAP_TYPE(OpResultType) == Type_float4)
#define IsIntResultType (MAP_TYPE(OpResultType) == Type_int || MAP_TYPE(OpResultType) == Type_int2 || MAP_TYPE(OpResultType) == Type_int3 || MAP_TYPE(OpResultType) == Type_int4)
#define IsUintResultType (MAP_TYPE(OpResultType) == Type_uint || MAP_TYPE(OpResultType) == Type_uint2 || MAP_TYPE(OpResultType) == Type_uint3 || MAP_TYPE(OpResultType) == Type_uint4)

#if OpCode == OpFDiv || OpCode == OpUDiv || OpCode == OpSDiv || OpCode == OpIAdd || OpCode == OpFAdd || OpCode == OpISub ||                 \
OpCode == OpFSub || OpCode == OpIMul || OpCode == OpFMul || OpCode == OpBitwiseAnd || OpCode == OpBitwiseOr || OpCode == OpBitwiseXor ||    \
OpCode == Pow || OpCode == Step || OpCode == OpLogicalOr || OpCode == OpLogicalAnd || OpCode == NMin || OpCode == NMax || OpCode == UMax || \
OpCode == SMax || OpCode == UMin || OpCode == SMin || OpCode == OpUMod || OpCode == OpSRem || OpCode == OpFRem || OpCode == OpShiftRightLogical || OpCode == OpShiftRightArithmetic || OpCode == OpShiftLeftLogical || \
OpCode == Cross || OpCode == Reflect
struct Op
{
    OpResultType Result;
    OpResultType Operand1;
    OpResultType Operand2;
};
#elif OpCode == OpIEqual || OpCode == OpINotEqual || OpCode == OpSGreaterThan || OpCode == OpSLessThan || OpCode == OpFOrdLessThan || OpCode == OpFOrdGreaterThan || \
OpCode == OpDot || OpCode == Distance
struct Op
{
    OpResultType Result;
    OpOperandType Operand1;
    OpOperandType Operand2;
};
#elif OpCode == SmoothStep || OpCode == FMix || OpCode == FClamp || OpCode == UClamp || OpCode == SClamp
struct Op
{
    OpResultType Result;
    OpResultType Operand1;
    OpResultType Operand2;
    OpResultType Operand3;
};
#elif OpCode == OpVectorTimesScalar
struct Op
{    
    OpResultType Result;
    OpOperandType1 Operand1;
    OpOperandType2 Operand2;
};
#elif OpCode == OpFNegate || OpCode == OpLogicalNot || OpCode == OpNot || OpCode == Sin || OpCode == Cos || OpCode == Tan || OpCode == Asin || OpCode == Acos || OpCode == Atan || OpCode == Sqrt ||  \
OpCode == Fract || OpCode == Floor || OpCode == Ceil || OpCode == FAbs || OpCode == SAbs || OpCode == RoundEven || OpCode == Exp || OpCode == Log || OpCode == Normalize || OpCode == Trunc ||        \
OpCode == Exp2 || OpCode == Log2 || OpCode == InverseSqrt
struct Op
{
    OpResultType Result;
    OpResultType Operand;
};
#elif OpCode == OpConvertFToU || OpCode == OpConvertFToS || OpCode == OpConvertSToF || OpCode == OpConvertUToF || OpCode == OpIsNan || OpCode == OpIsInf || OpCode == Length ||  \
OpCode == OpAny || OpCode == OpAll || OpCode == OpBitcast
struct Op
{
    OpResultType Result;
    OpOperandType Operand;
};
#elif OpCode == OpSelect
struct Op
{
    OpResultType Result;
    OpConditionType Condition;
    OpResultType Object1;
    OpResultType Object2;  
};
#elif OpCode == OpDPdx || OpCode == OpDPdy || OpCode == OpFwidth
struct Op
{
    OpResultType Result[4];
    OpResultType Operands[4];
};
#elif OpCode == OpImageSampleImplicitLod
struct Op
{
    OpResultType Result[4];
    OpOperandType Operands[4];
};
#endif 

RWStructuredBuffer<Op> Input : register(u0, space0);

#if OpCode == OpImageSampleImplicitLod
    Texture2D Image : register(t1, space0);
    SamplerState Sampler : register(s2, space0);
#endif

[numthreads(1, 1, 1)]
void MainCS()
{
#if OpCode == OpBitwiseAnd
    Input[0].Result = Input[0].Operand1 & Input[0].Operand2;
#elif OpCode == OpBitwiseOr
    Input[0].Result = Input[0].Operand1 | Input[0].Operand2;
#elif OpCode == OpBitwiseXor
    Input[0].Result = Input[0].Operand1 ^ Input[0].Operand2;
#elif OpCode == OpLogicalOr
    Input[0].Result = or(Input[0].Operand1,Input[0].Operand2);
#elif OpCode == OpLogicalAnd
    Input[0].Result = and(Input[0].Operand1,Input[0].Operand2);
#elif OpCode == OpLogicalNot
    Input[0].Result = !Input[0].Operand;
#elif OpCode == OpShiftRightLogical || OpCode == OpShiftRightArithmetic
    Input[0].Result = Input[0].Operand1 >> Input[0].Operand2;
#elif OpCode == OpShiftLeftLogical
    Input[0].Result = Input[0].Operand1 << Input[0].Operand2;
#elif OpCode == OpSelect
    Input[0].Result = select(Input[0].Condition,Input[0].Object1,Input[0].Object2);
#elif OpCode == OpIMul || OpCode == OpFMul || OpCode == OpVectorTimesScalar
    Input[0].Result = Input[0].Operand1 * Input[0].Operand2;
#elif OpCode == OpIEqual
    Input[0].Result = Input[0].Operand1 == Input[0].Operand2;
#elif OpCode == OpINotEqual
    Input[0].Result = Input[0].Operand1 != Input[0].Operand2;
#elif OpCode == OpSGreaterThan || OpCode == OpFOrdGreaterThan
    Input[0].Result = Input[0].Operand1 > Input[0].Operand2;
#elif OpCode == OpSLessThan || OpCode == OpFOrdLessThan
    Input[0].Result = Input[0].Operand1 < Input[0].Operand2;
#elif OpCode == OpConvertFToU || OpCode == OpConvertFToS || OpCode == OpConvertSToF || OpCode == OpConvertUToF
    Input[0].Result = (OpResultType)Input[0].Operand;
#elif OpCode == OpDot
    Input[0].Result = dot(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == OpAny
    Input[0].Result = any(Input[0].Operand);
#elif OpCode == OpAll
    Input[0].Result = all(Input[0].Operand);
#elif OpCode == OpIsNan
    Input[0].Result = isnan(Input[0].Operand);
#elif OpCode == OpIsInf
    Input[0].Result = isinf(Input[0].Operand);
#elif OpCode == OpFNegate
    Input[0].Result = -Input[0].Operand;
#elif OpCode == OpNot
    Input[0].Result = ~Input[0].Operand;
#elif OpCode == OpIAdd || OpCode == OpFAdd
    Input[0].Result = Input[0].Operand1 + Input[0].Operand2;
#elif OpCode == OpISub || OpCode == OpFSub
    Input[0].Result = Input[0].Operand1 - Input[0].Operand2;
#elif OpCode == OpFDiv || OpCode == OpUDiv || OpCode == OpSDiv
    Input[0].Result = Input[0].Operand1 / Input[0].Operand2;
#elif OpCode == OpUMod || OpCode == OpSRem || OpCode == OpFRem
    Input[0].Result = Input[0].Operand1 % Input[0].Operand2;
#elif OpCode == RoundEven
    Input[0].Result = round(Input[0].Operand);
#elif OpCode == Trunc
    Input[0].Result = trunc(Input[0].Operand);
#elif OpCode == FAbs || OpCode == SAbs
    Input[0].Result = abs(Input[0].Operand);
#elif OpCode == Floor
    Input[0].Result = floor(Input[0].Operand);
#elif OpCode == Ceil
    Input[0].Result = ceil(Input[0].Operand);
#elif OpCode == Fract
    Input[0].Result = frac(Input[0].Operand);
#elif OpCode == Sin
    Input[0].Result = sin(Input[0].Operand);
#elif OpCode == Cos
    Input[0].Result = cos(Input[0].Operand);
#elif OpCode == Tan
    Input[0].Result = tan(Input[0].Operand);
#elif OpCode == Asin
    Input[0].Result = asin(Input[0].Operand);
#elif OpCode == Acos
    Input[0].Result = acos(Input[0].Operand);
#elif OpCode == Atan
    Input[0].Result = atan(Input[0].Operand);
#elif OpCode == Pow
    Input[0].Result = pow(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == Exp
    Input[0].Result = exp(Input[0].Operand);
#elif OpCode == Log
    Input[0].Result = log(Input[0].Operand);
#elif OpCode == Exp2
    Input[0].Result = exp2(Input[0].Operand);
#elif OpCode == Log2
    Input[0].Result = log2(Input[0].Operand);
#elif OpCode == Sqrt
    Input[0].Result = sqrt(Input[0].Operand);
#elif OpCode == InverseSqrt
    Input[0].Result = rsqrt(Input[0].Operand);
#elif OpCode == FMix
    Input[0].Result = lerp(Input[0].Operand1, Input[0].Operand2, Input[0].Operand3);
#elif OpCode == Step
    Input[0].Result = step(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == SmoothStep
    Input[0].Result = smoothstep(Input[0].Operand1, Input[0].Operand2, Input[0].Operand3);
#elif OpCode == Length
    Input[0].Result = length(Input[0].Operand);
#elif OpCode == Distance
    Input[0].Result = distance(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == Cross
    Input[0].Result = cross(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == Normalize
    Input[0].Result = normalize(Input[0].Operand);
#elif OpCode == Reflect
    Input[0].Result = reflect(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == NMin || OpCode == UMin || OpCode == SMin
    Input[0].Result = min(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == NMax || OpCode == UMax || OpCode == SMax
    Input[0].Result = max(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == FClamp || OpCode == UClamp || OpCode == SClamp
    Input[0].Result = clamp(Input[0].Operand1, Input[0].Operand2, Input[0].Operand3);
#elif OpCode == OpBitcast && IsFloatResultType
    Input[0].Result = asfloat(Input[0].Operand);
#elif OpCode == OpBitcast && IsIntResultType
    Input[0].Result = asint(Input[0].Operand);
#elif OpCode == OpBitcast && IsUintResultType
    Input[0].Result = asuint(Input[0].Operand);
#endif
}

void MainVS(in uint VertID : SV_VertexID, out float4 Pos : SV_Position)
{
    float2 uv = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
}

void MainPS(in float4 Pos : SV_Position)
{
    int QuadIndex = 2 * (int(Pos.y) & 1) + (int(Pos.x) & 1);
#if OpCode == OpDPdx
    OpResultType Operand = Input[0].Operands[QuadIndex];
    Input[0].Result[QuadIndex] = ddx(Operand);
#elif OpCode == OpDPdy
    OpResultType Operand = Input[0].Operands[QuadIndex];
    Input[0].Result[QuadIndex] = ddy(Operand);
#elif OpCode == OpFwidth
    OpResultType Operand = Input[0].Operands[QuadIndex];
    Input[0].Result[QuadIndex] = fwidth(Operand);
#elif OpCode == OpImageSampleImplicitLod
    Input[0].Result[QuadIndex] = Image.Sample(Sampler, Input[0].Operands[QuadIndex]);
#endif
}