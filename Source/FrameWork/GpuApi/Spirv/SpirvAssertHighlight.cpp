#include "CommonHeader.h"
#include "SpirvAssertHighlight.h"

namespace FW
{
	SpvId SpvAssertHighlightVisitor::FindAssertResultVar() const
	{
		for (const auto& [Id, Name] : Context.Names)
		{
			if (Name == TEXT("GPrivate_AssertResult"))
			{
				auto It = Context.GlobalVariables.find(Id);
				if (It != Context.GlobalVariables.end() && It->second.StorageClass == SpvStorageClass::Private)
				{
					return Id;
				}
			}
		}
		return {};
	}

	void SpvAssertHighlightVisitor::CollectLocationOutputs(TArray<SpvId>& OutOutputs) const
	{
		for (const auto& [Id, Var] : Context.GlobalVariables)
		{
			if (Var.StorageClass != SpvStorageClass::Output)
			{
				continue;
			}
			TArray<SpvDecoration> Decs;
			Context.Decorations.MultiFind(Id, Decs);
			for (const SpvDecoration& Dec : Decs)
			{
				if (Dec.Kind == SpvDecorationKind::Location)
				{
					OutOutputs.Add(Id);
					break;
				}
			}
		}
	}

	SpvId SpvAssertHighlightVisitor::BuildPinkConstantForType(SpvType* PointeeType)
	{
		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId SIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));

		auto BuildFloatPink = [&](uint32 Count) -> SpvId
		{
			SpvId One = Patcher.FindOrAddConstant(1.0f);
			SpvId Zero = Patcher.FindOrAddConstant(0.0f);
			if (Count == 1)
			{
				return One;
			}
			SpvId VecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, Count));
			TArray<SpvId> Members;
			// Pink = (1,0,1,1). For 2-component -> (1,0). For 3-component -> (1,0,1).
			Members.Add(One);
			if (Count >= 2) Members.Add(Zero);
			if (Count >= 3) Members.Add(One);
			if (Count >= 4) Members.Add(One);
			return Patcher.FindOrAddConstant(MakeUnique<SpvOpConstantComposite>(VecType, Members));
		};

		auto BuildUIntPink = [&](uint32 Count) -> SpvId
		{
			SpvId One = Patcher.FindOrAddConstant(1u);
			SpvId Zero = Patcher.FindOrAddConstant(0u);
			if (Count == 1)
			{
				return One;
			}
			SpvId VecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, Count));
			TArray<SpvId> Members;
			Members.Add(One);
			if (Count >= 2) Members.Add(Zero);
			if (Count >= 3) Members.Add(One);
			if (Count >= 4) Members.Add(One);
			return Patcher.FindOrAddConstant(MakeUnique<SpvOpConstantComposite>(VecType, Members));
		};

		auto BuildSIntPink = [&](uint32 Count) -> SpvId
		{
			SpvId One = Patcher.FindOrAddConstant(1);
			SpvId Zero = Patcher.FindOrAddConstant(0);
			if (Count == 1)
			{
				return One;
			}
			SpvId VecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(SIntType, Count));
			TArray<SpvId> Members;
			Members.Add(One);
			if (Count >= 2) Members.Add(Zero);
			if (Count >= 3) Members.Add(One);
			if (Count >= 4) Members.Add(One);
			return Patcher.FindOrAddConstant(MakeUnique<SpvOpConstantComposite>(VecType, Members));
		};

		switch (PointeeType->GetKind())
		{
		case SpvTypeKind::Float:
			return BuildFloatPink(1);
		case SpvTypeKind::Integer:
		{
			auto* IntT = static_cast<SpvIntegerType*>(PointeeType);
			return IntT->IsSigend() ? BuildSIntPink(1) : BuildUIntPink(1);
		}
		case SpvTypeKind::Vector:
		{
			auto* VecT = static_cast<SpvVectorType*>(PointeeType);
			SpvScalarType* Elem = VecT->ElementType;
			if (Elem->GetKind() == SpvTypeKind::Float)
			{
				return BuildFloatPink(VecT->ElementCount);
			}
			if (Elem->GetKind() == SpvTypeKind::Integer)
			{
				auto* IntT = static_cast<SpvIntegerType*>(Elem);
				return IntT->IsSigend() ? BuildSIntPink(VecT->ElementCount) : BuildUIntPink(VecT->ElementCount);
			}
		}
		default:
			AUX::Unreachable();
		}
	}

	void SpvAssertHighlightVisitor::AppendPixelFailBranch(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		for (SpvId OutputId : LocationOutputs)
		{
			SpvVariable& Var = Context.GlobalVariables[OutputId];
			SpvType* PointeeType = Var.PointerType ? Var.PointerType->PointeeType : nullptr;
			if (!PointeeType)
			{
				continue;
			}
			SpvId Pink = BuildPinkConstantForType(PointeeType);
			InstList.Add(MakeUnique<SpvOpStore>(OutputId, Pink));
		}
		InstList.Add(MakeUnique<SpvOpReturn>());
	}

	void SpvAssertHighlightVisitor::PatchAssertTerminator(SpvInstruction* Term, void (SpvAssertHighlightVisitor::*AppendFailBody)(TArray<TUniquePtr<SpvInstruction>>&))
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId OneU = Patcher.FindOrAddConstant(1u);

		TArray<TUniquePtr<SpvInstruction>> Pre;

		SpvId LoadedAssert = Patcher.NewId();
		{
			auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, AssertResultVar);
			LoadOp->SetId(LoadedAssert);
			Pre.Add(MoveTemp(LoadOp));
		}

		SpvId Failed = Patcher.NewId();
		{
			auto NeqOp = MakeUnique<SpvOpINotEqual>(BoolType, LoadedAssert, OneU);
			NeqOp->SetId(Failed);
			Pre.Add(MoveTemp(NeqOp));
		}

		SpvId FailLbl = Patcher.NewId();
		SpvId PassLbl = Patcher.NewId();
		SpvId MergeLbl = Patcher.NewId();

		Pre.Add(MakeUnique<SpvOpSelectionMerge>(MergeLbl, SpvSelectionControl::None));
		Pre.Add(MakeUnique<SpvOpBranchConditional>(Failed, FailLbl, PassLbl));

		{
			auto FailLabel = MakeUnique<SpvOpLabel>();
			FailLabel->SetId(FailLbl);
			Pre.Add(MoveTemp(FailLabel));
		}

		(this->*AppendFailBody)(Pre);

		{
			auto PassLabel = MakeUnique<SpvOpLabel>();
			PassLabel->SetId(PassLbl);
			Pre.Add(MoveTemp(PassLabel));
		}

		TArray<TUniquePtr<SpvInstruction>> Post;
		{
			auto MergeLabel = MakeUnique<SpvOpLabel>();
			MergeLabel->SetId(MergeLbl);
			Post.Add(MoveTemp(MergeLabel));
		}
		Post.Add(MakeUnique<SpvOpReturn>());

		Patcher.AddInstructions(Term->GetWordOffset().value() + Term->GetWordLen().value(), MoveTemp(Post));
		Patcher.AddInstructions(Term->GetWordOffset().value(), MoveTemp(Pre));
	}

	void SpvAssertHighlightVisitor::PatchPixelTerminator(SpvInstruction* Term)
	{
		PatchAssertTerminator(Term, &SpvAssertHighlightVisitor::AppendPixelFailBranch);
	}

	SpvId SpvAssertHighlightVisitor::DeclareAssertThreadBuffer()
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UVec3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 3));
		SpvId RunTimeArrayType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeRuntimeArray>(UVec3Type));
		SpvId BufferType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>(TArray<SpvId>{RunTimeArrayType}));
		SpvId BufferPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, BufferType));

		int ArrayStride = 16;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(RunTimeArrayType, SpvDecorationKind::ArrayStride, TArray<uint8>{ (uint8*)&ArrayStride, sizeof(int) }));
		int MemberOffset = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(BufferType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(BufferType, SpvDecorationKind::BufferBlock));

		SpvId Var = Patcher.NewId();
		auto VarOp = MakeUnique<SpvOpVariable>(BufferPointerType, SpvStorageClass::Uniform);
		VarOp->SetId(Var);
		Patcher.AddGlobalVariable(MoveTemp(VarOp));
		Patcher.AddDebugName(MakeUnique<SpvOpName>(Var, "_AssertThreadBuffer_"));

		int SetNumber = AssertHighlightBindGroupSlot;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(Var, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = GetSpirvPatchBindingNumber(AssertThreadBufferBindingSlot, BindingType::RWRawBuffer, BindingShaderStage::Compute);
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(Var, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));
		return Var;
	}

	void SpvAssertHighlightVisitor::AppendComputeFailReport(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 3));
		SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
		SpvId UVec3PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UInt3Type));
		SpvId Zero = Patcher.FindOrAddConstant(0u);

		SpvId LoadedGID = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UInt3Type, GlobalInvocationIdVar);
			Op->SetId(LoadedGID);
			InstList.Add(MoveTemp(Op));
		}

		SpvId CounterPtr = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, AssertThreadBufferVar, TArray<SpvId>{Zero, Zero, Zero});
			Op->SetId(CounterPtr);
			InstList.Add(MoveTemp(Op));
		}
		SpvId RecordIndex = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAtomicIAdd>(UIntType, CounterPtr,
				Patcher.FindOrAddConstant((uint32)SpvMemoryScope::Device),
				Patcher.FindOrAddConstant((uint32)SpvMemorySemantics::None),
				Patcher.FindOrAddConstant(1u));
			Op->SetId(RecordIndex);
			InstList.Add(MoveTemp(Op));
		}
		SpvId RecordStoreIndex = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpIAdd>(UIntType, RecordIndex, Patcher.FindOrAddConstant(1u));
			Op->SetId(RecordStoreIndex);
			InstList.Add(MoveTemp(Op));
		}
		SpvId RecordPtr = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpAccessChain>(UVec3PointerUniformType, AssertThreadBufferVar, TArray<SpvId>{Zero, RecordStoreIndex});
			Op->SetId(RecordPtr);
			InstList.Add(MoveTemp(Op));
		}
		InstList.Add(MakeUnique<SpvOpStore>(RecordPtr, LoadedGID));

		InstList.Add(MakeUnique<SpvOpReturn>());
	}

	void SpvAssertHighlightVisitor::PatchComputeTerminator(SpvInstruction* Term)
	{
		PatchAssertTerminator(Term, &SpvAssertHighlightVisitor::AppendComputeFailReport);
	}

	void SpvAssertHighlightVisitor::Parse(TArray<TUniquePtr<SpvInstruction>>& InInsts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections)
	{
		Insts = &InInsts;
		Patcher.SetSpvContext(InInsts, SpvCode, &Context);

		RemapSpvDebuggerBindings(InInsts, Patcher, Context, RuntimeBindings);

		AssertResultVar = FindAssertResultVar();
		if (!AssertResultVar.IsValid())
		{
			// Shader has no assertions; nothing to patch.
			return;
		}

		if (Stage == ShaderType::Pixel)
		{
			CollectLocationOutputs(LocationOutputs);
			if (LocationOutputs.IsEmpty())
			{
				return;
			}
		}
		else if (Stage == ShaderType::Compute)
		{
			AssertThreadBufferVar = DeclareAssertThreadBuffer();
			GlobalInvocationIdVar = PatchGlobalInvocationIdBuiltIn(Patcher, Context);
		}
		else
		{
			return;
		}

		// Walk the entry-point function body and patch every OpReturn / OpKill.
		const int32 EntryIndex = GetInstIndex(Insts, Context.EntryPoint);
		for (int32 i = EntryIndex; i < Insts->Num(); i++)
		{
			SpvInstruction* Inst = (*Insts)[i].Get();
			if (dynamic_cast<const SpvOpFunctionEnd*>(Inst))
			{
				break;
			}

			const bool bIsReturn = dynamic_cast<const SpvOpReturn*>(Inst) != nullptr;
			const bool bIsKill = dynamic_cast<const SpvOpKill*>(Inst) != nullptr;
			if (!bIsReturn && !bIsKill)
			{
				continue;
			}

			if (!Inst->GetWordOffset().has_value())
			{
				continue;
			}

			if (Stage == ShaderType::Pixel)
			{
				PatchPixelTerminator(Inst);
			}
			else
			{
				PatchComputeTerminator(Inst);
			}
		}
	}
}
