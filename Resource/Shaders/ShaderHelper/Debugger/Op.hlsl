#define OpConvertSToF 111
#define OpFNegate 127
#define OpFAdd 129
#define OpISub 130
#define OpFSub 131
#define OpFDiv 136
#define IEqual 170
#define	INotEqual 171
#define	SGreaterThan 173
#define SLessThan 177
#define FOrdLessThan 184
#define FOrdGreaterThan 186
#define Sin 13
#define Cos 14

#if OpCode == OpFDiv || OpCode == OpFAdd || OpCode == OpISub || OpCode == OpFSub
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
#elif OpCode == OpFNegate || OpCode == Sin || OpCode == Cos
struct Op
{
    OpResultType Result;
    OpResultType Operand;
};
#elif OpCode == OpConvertSToF
struct Op
{
    OpResultType Result;
    OpOperandType Operand;
};
#endif 

RWStructuredBuffer<Op> Input : register(u0, space0);

[numthreads(1, 1, 1)]
void MainCS()
{
#if OpCode == IEqual
    Input[0].Result = Input[0].Operand1 == Input[0].Operand2;
#elif OpCode == INotEqual
    Input[0].Result = Input[0].Operand1 != Input[0].Operand2;
#elif OpCode == SGreaterThan || OpCode == FOrdGreaterThan
    Input[0].Result = Input[0].Operand1 > Input[0].Operand2;
#elif OpCode == SLessThan || OpCode == FOrdLessThan
    Input[0].Result = Input[0].Operand1 < Input[0].Operand2;
#elif OpCode == OpConvertSToF
    Input[0].Result = (OpResultType)Input[0].Operand;
#elif OpCode == OpFAdd
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