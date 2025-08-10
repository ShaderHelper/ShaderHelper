#define OpImageSampleImplicitLod 87
#define OpConvertFToU 109
#define OpConvertFToS 110
#define OpConvertSToF 111
#define OpConvertUToF 112
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
#define OpBitwiseOr 197
#define OpBitwiseXor 198
#define OpBitwiseAnd 199
#define OpNot 200
#define OpDPdx 207
#define OpDPdy 208

#define RoundEven 2
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
#define Sqrt 31
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
#define Normalize 69
#define NMin 79
#define NMax 80

#if OpCode == OpFDiv || OpCode == OpUDiv || OpCode == OpSDiv || OpCode == OpIAdd || OpCode == OpFAdd || OpCode == OpISub ||                 \
OpCode == OpFSub || OpCode == OpIMul || OpCode == OpFMul || OpCode == OpBitwiseAnd || OpCode == OpBitwiseOr || OpCode == OpBitwiseXor ||    \
OpCode == Pow || OpCode == Step || OpCode == OpLogicalOr || OpCode == OpLogicalAnd || OpCode == NMin || OpCode == NMax || OpCode == UMax || \
OpCode == SMax || OpCode == UMin || OpCode == SMin || OpCode == OpUMod || OpCode == OpSRem || OpCode == OpFRem
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
OpCode == Fract || OpCode == Floor || OpCode == Ceil || OpCode == FAbs || OpCode == SAbs || OpCode == RoundEven || OpCode == Exp || OpCode == Log || OpCode == Normalize
struct Op
{
    OpResultType Result;
    OpResultType Operand;
};
#elif OpCode == OpConvertFToU || OpCode == OpConvertFToS || OpCode == OpConvertSToF || OpCode == OpConvertUToF || OpCode == OpIsNan || OpCode == OpIsInf || OpCode == Length ||  \
OpCode == OpAny || OpCode == OpAll
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
#elif OpCode == OpDPdx || OpCode == OpDPdy
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
#elif OpCode == Sqrt
    Input[0].Result = sqrt(Input[0].Operand);
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
#elif OpCode == Normalize
    Input[0].Result = normalize(Input[0].Operand);
#elif OpCode == NMin || OpCode == UMin || OpCode == SMin
    Input[0].Result = min(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == NMax || OpCode == UMax || OpCode == SMax
    Input[0].Result = max(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == FClamp || OpCode == UClamp || OpCode == SClamp
    Input[0].Result = clamp(Input[0].Operand1, Input[0].Operand2, Input[0].Operand3);
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
#elif OpCode == OpImageSampleImplicitLod
    Input[0].Result[QuadIndex] = Image.Sample(Sampler, Input[0].Operands[QuadIndex]);
#endif
}