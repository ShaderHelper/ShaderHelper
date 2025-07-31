#define OpConvertFToS 110
#define OpConvertSToF 111
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
#define OpVectorTimesScalar 142
#define OpIsNan 156
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
#define OpDPdx 207
#define OpDPdy 208
#define Sin 13
#define Cos 14
#define Pow 26
#define Step 48
#define SmoothStep 49

#if OpCode == OpFDiv || OpCode == OpUDiv || OpCode == OpSDiv || OpCode == OpIAdd || OpCode == OpFAdd || OpCode == OpISub ||                 \
OpCode == OpFSub || OpCode == OpIMul || OpCode == OpFMul || OpCode == OpBitwiseAnd || OpCode == OpBitwiseOr || OpCode == OpBitwiseXor ||    \
OpCode == Pow || OpCode == Step
struct Op
{
    OpResultType Result;
    OpResultType Operand1;
    OpResultType Operand2;
};
#elif OpCode == OpIEqual || OpCode == OpINotEqual || OpCode == OpSGreaterThan || OpCode == OpSLessThan || OpCode == OpFOrdLessThan || OpCode == OpFOrdGreaterThan
struct Op
{
    OpResultType Result;
    OpOperandType Operand1;
    OpOperandType Operand2;
};
#elif OpCode == SmoothStep
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
#elif OpCode == OpFNegate || OpCode == Sin || OpCode == Cos
struct Op
{
    OpResultType Result;
    OpResultType Operand;
};
#elif OpCode == OpConvertFToS || OpCode == OpConvertSToF || OpCode == OpIsNan
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
#endif 

RWStructuredBuffer<Op> Input : register(u0, space0);

[numthreads(1, 1, 1)]
void MainCS()
{
#if OpCode == OpBitwiseAnd
    Input[0].Result = Input[0].Operand1 & Input[0].Operand2;
#elif OpCode == OpBitwiseOr
    Input[0].Result = Input[0].Operand1 | Input[0].Operand2;
#elif OpCode == OpBitwiseXor
    Input[0].Result = Input[0].Operand1 ^ Input[0].Operand
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
#elif OpCode == OpConvertFToS || OpCode == OpConvertSToF
    Input[0].Result = (OpResultType)Input[0].Operand;
#elif OpCode == OpIsNan
    Input[0].Result = isnan(Input[0].Operand);
#elif OpCode == OpFNegate
    Input[0].Result = -Input[0].Operand;
#elif OpCode == OpIAdd || OpCode == OpFAdd
    Input[0].Result = Input[0].Operand1 + Input[0].Operand2;
#elif OpCode == OpISub || OpCode == OpFSub
    Input[0].Result = Input[0].Operand1 - Input[0].Operand2;
#elif OpCode == OpFDiv || OpCode == OpUDiv || OpCode == OpSDiv
    Input[0].Result = Input[0].Operand1 / Input[0].Operand2;
#elif OpCode == Sin
    Input[0].Result = sin(Input[0].Operand);
#elif OpCode == Cos
    Input[0].Result = cos(Input[0].Operand);
#elif OpCode == Pow
    Input[0].Result = pow(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == Step
    Input[0].Result = step(Input[0].Operand1, Input[0].Operand2);
#elif OpCode == SmoothStep
    Input[0].Result = smoothstep(Input[0].Operand1, Input[0].Operand2, Input[0].Operand3);
#endif
}

void MainVS(in uint VertID : SV_VertexID, out float4 Pos : SV_Position)
{
    float2 uv = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
}

void MainPS(in float4 Pos : SV_Position)
{
#if OpCode == OpDPdx
    int QuadIndex = 2 * (int(Pos.y) & 1) + (int(Pos.x) & 1);
    OpResultType Operand = Input[0].Operands[QuadIndex];
    Input[0].Result[QuadIndex] = ddx(Operand);
#elif OpCode == OpDPdy
    int QuadIndex = 2 * (int(Pos.y) & 1) + (int(Pos.x) & 1);
    OpResultType Operand = Input[0].Operands[QuadIndex];
    Input[0].Result[QuadIndex] = ddy(Operand);
#endif
}