#include "CommonHeader.h"
#include "SpirvDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	void SpvDebuggerVisitor::Visit(const SpvDebugDeclare* Inst)
	{
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		Context.VariableDescMap.emplace(Inst->GetVariable(),VarDesc);
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugValue* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvDebugLine* Inst)
	{
		CurLine = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineStart()].Storage).Value.GetData();
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugScope* Inst)
	{
		CurScope = Context.LexicalScopes[Inst->GetScope()].Get();

		TArray<TUniquePtr<SpvInstruction>> AppendScopeInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::ScopeChange);
			SpvId ScopeId = Patcher.FindOrAddConstant(CurScope->GetId().GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendScopeFuncId, TArray<SpvId>{ StateType, ScopeId});
			FuncCallOp->SetId(Patcher.NewId());
			AppendScopeInsts.Add(MoveTemp(FuncCallOp));
		}

		int EndOffset = Inst->GetWordOffset().value() + Inst->GetWordLen().value();
		Patcher.AddInstructions(EndOffset, MoveTemp(AppendScopeInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionCall* Inst)
	{
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
		{
			TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::FuncCall);
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendTagInsts));
		}

		{
			TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::FuncCallAfterReturn);
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
			Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(AppendTagInsts));
		}

		if(CurScope)
		{
			TArray<TUniquePtr<SpvInstruction>> AppendScopeInsts;
			{
				SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::ScopeChange);
				SpvId ScopeId = Patcher.FindOrAddConstant(CurScope->GetId().GetValue());
				SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
				auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendScopeFuncId, TArray<SpvId>{ StateType, ScopeId});
				FuncCallOp->SetId(Patcher.NewId());
				AppendScopeInsts.Add(MoveTemp(FuncCallOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(AppendScopeInsts));
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionParameter* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = { {ResultId, PointerType->PointeeType}, SpvStorageClass::Function};

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

		Context.LocalVariables.emplace(ResultId, MoveTemp(Var));
		SpvPointer Pointer{
			.Id = ResultId,
			.Var = &Context.LocalVariables[ResultId],
			.Type = PointerType
		};
		Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpVariable* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, StorageClass, PointerType };

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{MoveTemp(Value)};
		
		Context.LocalVariables.emplace(ResultId, MoveTemp(Var));
		SpvPointer Pointer{
			.Id = ResultId,
			.Var = &Context.LocalVariables[ResultId],
			.Type = PointerType
		};
		Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpPhi* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpLabel* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpLoad* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpStore* Inst)
	{
		SpvPointer* Pointer = Context.FindPointer(Inst->GetPointer());
		PatchAppendVarFunc(Pointer, Pointer->Indexes.Num());
	}

	void SpvDebuggerVisitor::Visit(const SpvOpAccessChain* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());

		SpvPointer Pointer{
			.Id = ResultId,
			.Var = Context.FindPointer(Inst->GetBasePointer())->Var,
			.Indexes = Inst->GetIndexes(),
			.Type = PointerType
		};
		Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToU* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToS* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUDiv* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSDiv* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUMod* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSRem* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFRem* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranch* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranchConditional* Inst)
	{
		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Condition);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		int OpMergeOffset = (*Insts)[InstIndex - 1]->GetWordOffset().value();
		Patcher.AddInstructions(OpMergeOffset, MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturn* Inst)
	{
		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Return);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturnValue* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvPow* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvFClamp* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvUClamp* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvSClamp* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvSmoothStep* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvNormalize* Inst)
	{
		
	}

	int32 SpvDebuggerVisitor::GetInstIndex(SpvId Inst) const
	{
		return Insts->IndexOfByPredicate([this, Inst](const TUniquePtr<SpvInstruction>& InInst){
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	void SpvDebuggerVisitor::PatchToDebugger(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvType* Type = Context.Types.at(InTypeId).Get();
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		if (InTypeId == UIntType)
		{
			SpvId LoadedDebuggerOffset = Patcher.NewId();
			auto LoadedDebuggerOffsetOp = MakeUnique<SpvOpLoad>(UIntType, DebuggerOffset);
			LoadedDebuggerOffsetOp->SetId(LoadedDebuggerOffset);
			InstList.Add(MoveTemp(LoadedDebuggerOffsetOp));

			SpvId AlignedDebuggerOffset = Patcher.NewId();
			auto AlignedDebuggerOffsetOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(2u));
			AlignedDebuggerOffsetOp->SetId(AlignedDebuggerOffset);
			InstList.Add(MoveTemp(AlignedDebuggerOffsetOp));

			SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
			SpvId DebuggerBufferStorage = Patcher.NewId();
			auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Patcher.FindOrAddConstant(0u), AlignedDebuggerOffset});
			DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
			InstList.Add(MoveTemp(DebuggerBufferStorageOp));

			InstList.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, InValueId));

			SpvId NewDebuggerOffset = Patcher.NewId();
			auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(4u));
			AddOp->SetId(NewDebuggerOffset);
			InstList.Add(MoveTemp(AddOp));

			InstList.Add(MakeUnique<SpvOpStore>(DebuggerOffset, NewDebuggerOffset));
		}
		else
		{
			if (Type->IsScalar())
			{
				SpvId BitCastValue = Patcher.NewId();
				auto BitCastOp = MakeUnique<SpvOpBitcast>(UIntType, InValueId);
				BitCastOp->SetId(BitCastValue);
				InstList.Add(MoveTemp(BitCastOp));
				PatchToDebugger(BitCastValue, UIntType, InstList);
			}
			else if (Type->GetKind() == SpvTypeKind::Vector)
			{

			}
		}

	}

	void SpvDebuggerVisitor::PatchAppendVarFunc(SpvPointer* Pointer, uint32 IndexNum)
	{
		SpvPointerType* PointerType = Pointer->Type;
		SpvType* PointeeType = Pointer->Type->PointeeType;
		if (AppendVarFuncIds.Contains({ PointeeType, IndexNum}))
		{
			return;
		}

		SpvId AppendVarFuncId = Patcher.NewId();
		FString TypeName;
		if (Context.Names.contains(PointeeType->GetId()))
		{
			TypeName = Context.Names[PointeeType->GetId()];
		}
		else
		{
			TypeName = GetHlslTypeStr(PointeeType);
		}
		FString FuncName = FString::Printf(TEXT("__AppendVar_%s_%d"), *TypeName, IndexNum);
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendVarFuncId, FuncName));
		TArray<TUniquePtr<SpvInstruction>> AppendVarFuncInsts;
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> ParamterTypes;
			for (uint32 i = 0; i < IndexNum + 3; i++)
			{
				ParamterTypes.Add(UIntType);
			}
			ParamterTypes.Add(PointeeType->GetId());

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendVarFuncId);
			AppendVarFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendVarFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "__DebuggerStateType"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendVarFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "__DebuggerLine"));

			SpvId VarIdParam = Patcher.NewId();
			auto VarIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			VarIdParamOp->SetId(VarIdParam);
			AppendVarFuncInsts.Add(MoveTemp(VarIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(VarIdParam, "__DebuggerVarId"));

			TArray<SpvId> IndexIds;
			for (uint32 i = 0; i < IndexNum; i++)
			{
				SpvId IndexIdParam = Patcher.NewId();
				auto IndexIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
				IndexIdParamOp->SetId(IndexIdParam);
				AppendVarFuncInsts.Add(MoveTemp(IndexIdParamOp));
				Patcher.AddDebugName(MakeUnique<SpvOpName>(IndexIdParam, FString::Printf(TEXT("__DebuggerIndex%d"), i)));
				IndexIds.Add(IndexIdParam);
			}

			SpvId ValueParam = Patcher.NewId();
			auto ValueParamOp = MakeUnique<SpvOpFunctionParameter>(PointeeType->GetId());
			ValueParamOp->SetId(ValueParam);
			AppendVarFuncInsts.Add(MoveTemp(ValueParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ValueParam, "__DebuggerValue"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendVarFuncInsts.Add(MoveTemp(LabelOp));

			PatchToDebugger(StateTypeParam, UIntType, AppendVarFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendVarFuncInsts);
			PatchToDebugger(VarIdParam, UIntType, AppendVarFuncInsts);
			PatchToDebugger(Patcher.FindOrAddConstant(IndexNum), UIntType, AppendVarFuncInsts);
			for (SpvId IndexId : IndexIds)
			{
				PatchToDebugger(IndexId, UIntType, AppendVarFuncInsts);
			}
			PatchToDebugger(Patcher.FindOrAddConstant((uint32)GetTypeByteSize(PointeeType)), UIntType, AppendVarFuncInsts);
			PatchToDebugger(ValueParam, PointeeType->GetId(), AppendVarFuncInsts);

			AppendVarFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendVarFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendVarFuncInsts));
		AppendVarFuncIds.Add({ PointeeType, IndexNum });
	}

	void SpvDebuggerVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections)
	{
		this->Insts = &Insts;
		Patcher.SetSpvContext(Insts, SpvCode, &Context);
		//Get entry point loc
		InstIndex = GetInstIndex(Context.EntryPoint);

		//Init global external/location variables
		for (auto& [Id, Var] : Context.GlobalVariables)
		{
			int32 SetNumber = INDEX_NONE;
			int32 BindingNumber = INDEX_NONE;
			TArray<SpvDecoration> Decorations;
			Context.Decorations.MultiFind(Id, Decorations);
			for (const auto& Decoration : Decorations)
			{
				if (Decoration.Kind == SpvDecorationKind::DescriptorSet)
				{
					SetNumber = Decoration.DescriptorSet.Number;
				}
				else if (Decoration.Kind == SpvDecorationKind::Binding)
				{
					BindingNumber = Decoration.Binding.Number;
				}
			}
			if (SetNumber != INDEX_NONE && BindingNumber != INDEX_NONE;
				SpvBinding * Binding = Context.Bindings.FindByPredicate([&](const SpvBinding& InItem) { return InItem.Binding == BindingNumber && InItem.DescriptorSet == SetNumber; }))
			{
				auto& Storage = std::get<SpvObject::External>(Var.Storage);
				Storage.Resource = Binding->Resource;
				if (Binding->Resource->GetType() == GpuResourceType::Buffer)
				{
					GpuBuffer* Buffer = static_cast<GpuBuffer*>(Binding->Resource.GetReference());
					uint8* Data = (uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
					Storage.Value = { Data, (int)Buffer->GetByteSize() };
					GGpuRhi->UnMapGpuBuffer(Buffer);
					Var.InitializedRanges.AddUnique({ 0, (int)Buffer->GetByteSize() });
				}
			}
		}

		//Patch debugger buffer
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32,0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		SpvId RunTimeArrayType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeRuntimeArray>(UIntType));
		SpvId DebuggerBufferType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>( TArray<SpvId>{RunTimeArrayType}));
		SpvId DebuggerBufferPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, DebuggerBufferType));

		int ArrayStride = 4;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(RunTimeArrayType, SpvDecorationKind::ArrayStride, TArray<uint8>{ (uint8*)&ArrayStride, sizeof(int) }));
		int MemberOffset = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(DebuggerBufferType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBufferType, SpvDecorationKind::BufferBlock));

		DebuggerBuffer = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>( DebuggerBufferPointerType, SpvStorageClass::Uniform);
			VarOp->SetId(DebuggerBuffer);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerBuffer, "__DebuggerBuffer"));
		}
		int SetNumber = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = 0721;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		DebuggerOffset = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerOffset);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerOffset, "__DebuggerOffset"));
		}

		//Patch AppendScope function
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		AppendScopeFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendScopeFuncId, "__AppendScope"));
		TArray<TUniquePtr<SpvInstruction>> AppendScopeFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType, UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>( VoidType, SpvFunctionControl::None, FuncType );
			FuncOp->SetId(AppendScopeFuncId);
			AppendScopeFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendScopeFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "__DebuggerStateType"));

			SpvId ScopeIdParam = Patcher.NewId();
			auto ScopeIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			ScopeIdParamOp->SetId(ScopeIdParam);
			AppendScopeFuncInsts.Add(MoveTemp(ScopeIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ScopeIdParam, "__DebuggerScopeId"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendScopeFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendScopeFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendScopeFuncInsts);
			PatchToDebugger(ScopeIdParam, UIntType, AppendScopeFuncInsts);

			AppendScopeFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendScopeFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendScopeFuncInsts));

		AppendTagFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendTagFuncId, "__AppendTag"));
		TArray<TUniquePtr<SpvInstruction>> AppendTagFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType, UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendTagFuncId);
			AppendTagFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendTagFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "__DebuggerStateType"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendTagFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "__DebuggerLine"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendTagFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendTagFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendTagFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendTagFuncInsts);

			AppendTagFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendTagFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendTagFuncInsts));

		while (InstIndex < Insts.Num())
		{
			Insts[InstIndex]->Accept(this);
			InstIndex++;
		}

	}
}
