#define OpFNegate 127
#define OpFDiv 136

#if OpCode == OpFDiv
struct Op
{
    OpResultType Result;
    OpResultType Operand1;
    OpResultType Operand2;
};
#elif OpCode == OpFNegate
struct Op
{
    OpResultType Result;
    OpResultType Operand;
};
#else
#endif 

RWStructuredBuffer<Op> Input : register(u0, space0);

[numthreads(1, 1, 1)]
void MainCS()
{
#if OpCode == OpFDiv
    Input[0].Result = Input[0].Operand1 / Input[0].Operand2;
#endif
}