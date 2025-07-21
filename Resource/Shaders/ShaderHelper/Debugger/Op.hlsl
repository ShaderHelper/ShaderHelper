#define OpConvertFToS 110
#define OpConvertSToF 111
#define OpFNegate 127
#define OpIAdd 128
#define OpFAdd 129
#define OpISub 130
#define OpFSub 131
#define OpIMul 132
#define OpFMul 133
#define OpFDiv 136
#define OpVectorTimesScalar 142
#define OpSelect 169
#define IEqual 170
#define	INotEqual 171
#define	SGreaterThan 173
#define SLessThan 177
#define FOrdLessThan 184
#define FOrdGreaterThan 186
#define OpBitwiseAnd 199
#define Sin 13
#define Cos 14

#if OpCode == OpFDiv || OpCode == OpIAdd || OpCode == OpFAdd || OpCode == OpISub || OpCode == OpFSub || OpCode == OpIMul || OpCode == OpFMul || OpCode == OpBitwiseAnd
struct Op
{
    OpResultType Result;
    OpResultType Operand1;
    OpResultType Operand2;
};
#elif OpCode == IEqual || OpCode == INotEqual || OpCode == SGreaterThan || OpCode == SLessThan || OpCode == FOrdLessThan || OpCode == FOrdGreaterThan
struct Op
{
    OpResultType Result;
    OpOperandType Operand1;
    OpOperandType Operand2;
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
#elif OpCode == OpConvertFToS || OpCode == OpConvertSToF
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
#endif 

RWStructuredBuffer<Op> Input : register(u0, space0);

[numthreads(1, 1, 1)]
void MainCS()
{
#if OpCode == OpBitwiseAnd
    Input[0].Result = Input[0].Operand1 & Input[0].Operand2;
#elif OpCode == OpSelect
    Input[0].Result = select(Input[0].Condition,Input[0].Object1,Input[0].Object2);
#elif OpCode == OpIMul || OpCode == OpFMul || OpCode == OpVectorTimesScalar
    Input[0].Result = Input[0].Operand1 * Input[0].Operand2;
#elif OpCode == IEqual
    Input[0].Result = Input[0].Operand1 == Input[0].Operand2;
#elif OpCode == INotEqual
    Input[0].Result = Input[0].Operand1 != Input[0].Operand2;
#elif OpCode == SGreaterThan || OpCode == FOrdGreaterThan
    Input[0].Result = Input[0].Operand1 > Input[0].Operand2;
#elif OpCode == SLessThan || OpCode == FOrdLessThan
    Input[0].Result = Input[0].Operand1 < Input[0].Operand2;
#elif OpCode == OpConvertFToS || OpCode == OpConvertSToF
    Input[0].Result = (OpResultType)Input[0].Operand;
#elif OpCode == OpIAdd || OpCode == OpFAdd
    Input[0].Result = Input[0].Operand1 + Input[0].Operand2;
#elif OpCode == OpISub || OpCode == OpFSub
    Input[0].Result = Input[0].Operand1 - Input[0].Operand2;
#elif OpCode == OpFDiv
    Input[0].Result = Input[0].Operand1 / Input[0].Operand2;
#elif OpCode == Sin
    Input[0].Result = sin(Input[0].Operand);
#elif OpCode == Cos
    Input[0].Result = cos(Input[0].Operand);
#endif
}