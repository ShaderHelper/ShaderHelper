#include "CommonHeader.h"
#include "SpirvComputeDebugger.h"

namespace FW
{
	SpvComputeDebuggerVisitor::SpvComputeDebuggerVisitor(SpvComputeDebuggerContext& InComputeContext, GpuShaderLanguage InLanguage, bool InEnableUbsan)
		: SpvDebuggerVisitor(InComputeContext, InLanguage, ShaderType::Compute, InEnableUbsan)
	{
	}

	void SpvComputeDebuggerVisitor::ParseInternal()
	{
		WorkgroupIdVar = PatchWorkgroupIdBuiltIn(Patcher, Context);
		LocalInvocationIndexVar = PatchLocalInvocationIndexBuiltIn(Patcher, Context);
		DebuggerParams = PatchDebuggerParams(Patcher, ShaderType::Compute);

		SpvDebuggerVisitor::ParseInternal();
	}

	bool SpvComputeDebuggerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId Bool3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 3));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 3));

		// Load WorkgroupId (uvec3) built-in.
		SpvId LoadedWorkgroupId = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UInt3Type, WorkgroupIdVar);
			Op->SetId(LoadedWorkgroupId);
			InstList.Add(MoveTemp(Op));
		}

		// Load TargetWorkGroupId from uniform params.
		SpvId UInt3PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UInt3Type));
		SpvId TargetPtr = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAccessChain>(UInt3PointerUniformType, DebuggerParams, TArray<SpvId>{Patcher.FindOrAddConstant(0u)});
			Op->SetId(TargetPtr);
			InstList.Add(MoveTemp(Op));
		}
		SpvId LoadedTarget = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UInt3Type, TargetPtr);
			Op->SetId(LoadedTarget);
			InstList.Add(MoveTemp(Op));
		}

		// IEqual + All
		SpvId IEqual = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpIEqual>(Bool3Type, LoadedWorkgroupId, LoadedTarget);
			Op->SetId(IEqual);
			InstList.Add(MoveTemp(Op));
		}
		SpvId All = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAll>(BoolType, IEqual);
			Op->SetId(All);
			InstList.Add(MoveTemp(Op));
		}

		auto TrueLabel = Patcher.NewId();
		auto FalseLabel = Patcher.NewId();
		InstList.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
		InstList.Add(MakeUnique<SpvOpBranchConditional>(All, TrueLabel, FalseLabel));
		auto FalseLabelOp = MakeUnique<SpvOpLabel>();
		FalseLabelOp->SetId(FalseLabel);
		InstList.Add(MoveTemp(FalseLabelOp));
		InstList.Add(MakeUnique<SpvOpReturn>());
		auto TrueLabelOp = MakeUnique<SpvOpLabel>();
		TrueLabelOp->SetId(TrueLabel);
		InstList.Add(MoveTemp(TrueLabelOp));

		return true;
	}

	void SpvComputeDebuggerVisitor::BatchStoreToDebugBuffer(const TArray<SpvId>& UIntValues, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		if (UIntValues.IsEmpty())
		{
			return;
		}

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
		SpvId UVec4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 4));
		SpvId UVec4PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UVec4Type));
		SpvId Zero = Patcher.FindOrAddConstant(0u);

		int32 PayloadVec4Count = GetDebuggerBufferUVec4BatchCount(UIntValues.Num());
		// One uvec4 of prefix + payload.
		int32 ReservedBytes = (PayloadVec4Count + 1) * 16;

		// Atomically reserve a contiguous byte range. Counter lives at DebuggerBuffer.member0[0].x.
		SpvId CounterPtr = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, Zero, Zero});
			Op->SetId(CounterPtr);
			InstList.Add(MoveTemp(Op));
		}
		SpvId ByteOffset = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAtomicIAdd>(UIntType, CounterPtr,
				Patcher.FindOrAddConstant((uint32)SpvMemoryScope::Device),
				Patcher.FindOrAddConstant((uint32)SpvMemorySemantics::None),
				Patcher.FindOrAddConstant((uint32)ReservedBytes));
			Op->SetId(ByteOffset);
			InstList.Add(MoveTemp(Op));
		}

		// Convert byte offset to uvec4 index (offset / 16).
		SpvId RecordBaseIndex = Patcher.NewId();
		{
			auto ShiftOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, ByteOffset, Patcher.FindOrAddConstant(4u));
			ShiftOp->SetId(RecordBaseIndex);
			InstList.Add(MoveTemp(ShiftOp));
		}

		// Per-record prefix uvec4(LocalInvocationIndex, PayloadVec4Count, 0, 0) at RecordBaseIndex.
		{
			SpvId LocalIndex = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpLoad>(UIntType, LocalInvocationIndexVar);
				Op->SetId(LocalIndex);
				InstList.Add(MoveTemp(Op));
			}
			SpvId PayloadCountConst = Patcher.FindOrAddConstant((uint32)PayloadVec4Count);
			SpvId PrefixVec4 = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpCompositeConstruct>(UVec4Type, TArray<SpvId>{ LocalIndex, PayloadCountConst, Zero, Zero });
				Op->SetId(PrefixVec4);
				InstList.Add(MoveTemp(Op));
			}
			SpvId PrefixPtr = Patcher.NewId();
			{
				auto Op = MakeUnique<SpvOpAccessChain>(UVec4PointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, RecordBaseIndex});
				Op->SetId(PrefixPtr);
				InstList.Add(MoveTemp(Op));
			}
			InstList.Add(MakeUnique<SpvOpStore>(PrefixPtr, PrefixVec4));
		}

		// Payload base index = RecordBaseIndex + 1.
		SpvId PayloadBaseIndex = Patcher.NewId();
		{
			auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, RecordBaseIndex, Patcher.FindOrAddConstant(1u));
			AddOp->SetId(PayloadBaseIndex);
			InstList.Add(MoveTemp(AddOp));
		}

		StoreDebuggerBufferUVec4Batches(Patcher, DebuggerBuffer, PayloadBaseIndex, UIntValues, InstList);
	}

	void SpvComputeDebuggerVisitor::Visit(const SpvOpControlBarrier* Inst)
	{
		AppendTag([&] { return Inst->GetWordOffset().value() + Inst->GetWordLen().value(); }, SpvDebuggerStateType::WorkgroupBarrier);
	}
}
