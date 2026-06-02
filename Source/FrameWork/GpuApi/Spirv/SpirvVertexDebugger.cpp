#include "CommonHeader.h"
#include "SpirvVertexDebugger.h"

namespace FW
{
	void SpvVertexCaptureVisitor::Parse(TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>&)
	{
		Patcher.SetSpvContext(Insts, SpvCode, &Context);
		RemapSpvDebuggerBindings(Insts, Patcher, Context, Context.Bindings);
		SpvId DebuggerBuffer = PatchDebuggerBuffer(Patcher, ShaderType::Vertex);
		SpvId VertexIndexVar = PatchVertexIndexBuiltIn(Patcher, Context);
		SpvId InstanceIndexVar = PatchInstanceIndexBuiltIn(Patcher, Context);
		SpvId VertexCountConst = Patcher.FindOrAddConstant((uint32)VertexCount);

		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UVec4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 4));
		SpvId PositionVar = Context.BuiltIns[SpvBuiltIn::Position];

		// Insert a capture sequence before every OpReturn in the entry-point function.
		int32 EntryIndex = GetInstIndex(&Insts, Context.EntryPoint);
		for (int32 i = EntryIndex; i < Insts.Num(); i++)
		{
			if (dynamic_cast<const SpvOpFunctionEnd*>(Insts[i].Get()))
			{
				break;
			}
			auto* Ret = dynamic_cast<const SpvOpReturn*>(Insts[i].Get());
			if (!Ret)
			{
				continue;
			}

			TArray<TUniquePtr<SpvInstruction>> InstList;

			// Per-(instance,vertex) flat index: gl_InstanceIndex * VertexCount + gl_VertexIndex.
			SpvId LoadedVertexIndex = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpLoad>(UIntType, VertexIndexVar);
				Op->SetId(LoadedVertexIndex);
				InstList.Add(MoveTemp(Op));
			}
			SpvId LoadedInstanceIndex = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpLoad>(UIntType, InstanceIndexVar);
				Op->SetId(LoadedInstanceIndex);
				InstList.Add(MoveTemp(Op));
			}
			SpvId InstanceOffset = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpIMul>(UIntType, LoadedInstanceIndex, VertexCountConst);
				Op->SetId(InstanceOffset);
				InstList.Add(MoveTemp(Op));
			}
			SpvId BaseIndex = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpIAdd>(UIntType, InstanceOffset, LoadedVertexIndex);
				Op->SetId(BaseIndex);
				InstList.Add(MoveTemp(Op));
			}

			// float4 pos = gl_Position;
			SpvId LoadedPos = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpLoad>(Float4Type, PositionVar);
				Op->SetId(LoadedPos);
				InstList.Add(MoveTemp(Op));
			}

			// uvec4 posBits = bitcast(pos);
			SpvId BitcastPos = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpBitcast>(UVec4Type, LoadedPos);
				Op->SetId(BitcastPos);
				InstList.Add(MoveTemp(Op));
			}

			TArray<SpvId> UIntValues;
			for (uint32 Comp = 0; Comp < 4; Comp++)
			{
				SpvId CompId = Patcher.NewId();
				auto Op = MakeUnique<SpvOpCompositeExtract>(UIntType, BitcastPos, TArray<uint32>{ Comp });
				Op->SetId(CompId);
				InstList.Add(MoveTemp(Op));
				UIntValues.Add(CompId);
			}

			// One uvec4 per (instance,vertex) => buffer base index equals the flat index.
			StoreDebuggerBufferUVec4Batches(Patcher, DebuggerBuffer, BaseIndex, UIntValues, InstList);

			Patcher.AddInstructions(Ret->GetWordOffset().value(), MoveTemp(InstList));
		}
	}

	SpvVertexDebuggerVisitor::SpvVertexDebuggerVisitor(SpvVertexDebuggerContext& InContext, GpuShaderLanguage InLanguage, bool InEnableUbsan)
		: SpvDebuggerVisitor(InContext, InLanguage, ShaderType::Vertex, InEnableUbsan)
	{
	}

	void SpvVertexDebuggerVisitor::ParseInternal()
	{
		VertexIndexVar = PatchVertexIndexBuiltIn(Patcher, Context);
		InstanceIndexVar = PatchInstanceIndexBuiltIn(Patcher, Context);
		DebuggerParams = PatchDebuggerParams(Patcher, ShaderType::Vertex);

		SpvDebuggerVisitor::ParseInternal();
	}

	bool SpvVertexDebuggerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId Bool2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 2));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));

		// uvec2(gl_VertexIndex, gl_InstanceIndex)
		SpvId LoadedVertexIndex = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UIntType, VertexIndexVar);
			Op->SetId(LoadedVertexIndex);
			InstList.Add(MoveTemp(Op));
		}
		SpvId LoadedInstanceIndex = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UIntType, InstanceIndexVar);
			Op->SetId(LoadedInstanceIndex);
			InstList.Add(MoveTemp(Op));
		}
		SpvId CurrentId = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpCompositeConstruct>(UInt2Type, TArray<SpvId>{ LoadedVertexIndex, LoadedInstanceIndex });
			Op->SetId(CurrentId);
			InstList.Add(MoveTemp(Op));
		}

		// Load uvec2(TargetVertexIndex, TargetInstanceIndex) (member 0) from the debugger params buffer.
		SpvId UInt2PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UInt2Type));
		SpvId TargetPtr = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAccessChain>(UInt2PointerUniformType, DebuggerParams, TArray<SpvId>{Patcher.FindOrAddConstant(0u)});
			Op->SetId(TargetPtr);
			InstList.Add(MoveTemp(Op));
		}
		SpvId LoadedTarget = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UInt2Type, TargetPtr);
			Op->SetId(LoadedTarget);
			InstList.Add(MoveTemp(Op));
		}

		SpvId IEqual = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpIEqual>(Bool2Type, CurrentId, LoadedTarget);
			Op->SetId(IEqual);
			InstList.Add(MoveTemp(Op));
		}
		SpvId IsTarget = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAll>(BoolType, IEqual);
			Op->SetId(IsTarget);
			InstList.Add(MoveTemp(Op));
		}

		auto TrueLabel = Patcher.NewId();
		auto FalseLabel = Patcher.NewId();
		InstList.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
		InstList.Add(MakeUnique<SpvOpBranchConditional>(IsTarget, TrueLabel, FalseLabel));
		auto FalseLabelOp = MakeUnique<SpvOpLabel>();
		FalseLabelOp->SetId(FalseLabel);
		InstList.Add(MoveTemp(FalseLabelOp));
		InstList.Add(MakeUnique<SpvOpReturn>());
		auto TrueLabelOp = MakeUnique<SpvOpLabel>();
		TrueLabelOp->SetId(TrueLabel);
		InstList.Add(MoveTemp(TrueLabelOp));

		return true;
	}
}
