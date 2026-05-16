#pragma once
#include "../SpirvPixelDebugger.h"
#include "../SpirvComputeDebugger.h"

namespace FW
{
	struct SpvPixelExprDebuggerContext : SpvPixelDebuggerContext
	{
		SpvPixelExprDebuggerContext(const SpvDebugState& InStopDebugState, int32 InStopDebugStateIndex,
			const Vector2u& InCoord, const TArray<SpvBinding>& InBindings)
			: SpvPixelDebuggerContext{ InCoord, InBindings }
			, StopDebugState(InStopDebugState), StopDebugStateIndex(InStopDebugStateIndex)
		{}

		SpvDebugState StopDebugState;
		int32 StopDebugStateIndex;
	};

	struct SpvComputeExprDebuggerContext : SpvComputeDebuggerContext
	{
		SpvComputeExprDebuggerContext(const SpvDebugState& InStopDebugState, int32 InStopDebugStateIndex,
			const Vector3u& InTargetGroupId, const Vector3u& InThreadGroupSize, uint32 InSelectedLocalInvocationIndex,
			const TArray<SpvBinding>& InBindings)
			: SpvComputeDebuggerContext{ InTargetGroupId, InThreadGroupSize, InBindings }
			, StopDebugState(InStopDebugState), StopDebugStateIndex(InStopDebugStateIndex)
			, SelectedLocalInvocationIndex(InSelectedLocalInvocationIndex)
		{}

		SpvDebugState StopDebugState;
		int32 StopDebugStateIndex;
		uint32 SelectedLocalInvocationIndex;
	};

	template<typename TBase, typename TContext>
	class SpvExprDebuggerVisitorImpl : public TBase
	{
	public:
		using TBase::TBase;
		void Visit(const SpvDebugDeclare* Inst);

	protected:
		void ParseInternal() override;
		bool PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) override;
		virtual void PatchInvocationGate(TArray<TUniquePtr<SpvInstruction>>& InstList) {};

		void PatchBaseTypeAppendExprFunc();
		void PatchAppendExprFunc(const SpvType* Type, const SpvTypeDesc* TypeDesc);
		void PatchAppendExprDummyFunc();
		void AppendExprDummy(const TFunction<int32()>& OffsetEval);

		TMap<const SpvType*, SpvId> AppendExprFuncIds;
		SpvId DebugStateNum = 0;
		SpvId DoAppendExpr = 0;
		SpvId AppendExprDummyFuncId = 0;
	};

	template<typename TBase, typename TContext>
	void SpvExprDebuggerVisitorImpl<TBase, TContext>::Visit(const SpvDebugDeclare* Inst)
	{
		SpvDebuggerVisitor::Visit(Inst);

		SpvId VarId = Inst->GetVariable();
		SpvVariable* Var = this->Context.FindVar(VarId);

		//Insert an extra load instruction to prevent spirv-cross from optimizing away the variable.
		SpvId LoadedVar = this->Patcher.NewId();
		auto LoadedVarOp = MakeUnique<SpvOpLoad>(Var->Type->GetId(), VarId);
		LoadedVarOp->SetId(LoadedVar);
		this->Patcher.AddInstruction(Inst->GetWordOffset().value(), MoveTemp(LoadedVarOp));
	}

	template<typename TBase, typename TContext>
	void SpvExprDebuggerVisitorImpl<TBase, TContext>::ParseInternal()
	{
		auto& Patcher = this->Patcher;

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		SpvId ZeroU = Patcher.FindOrAddConstant(0u);

		DebugStateNum = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private, ZeroU);
			VarOp->SetId(DebugStateNum);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebugStateNum, "_DebugStateNum_"));
		}
		DoAppendExpr = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private, ZeroU);
			VarOp->SetId(DoAppendExpr);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DoAppendExpr, "_DoAppendExpr_"));
		}

		TBase::ParseInternal();

		PatchBaseTypeAppendExprFunc();
		for (const auto& [VarId, VarDesc] : this->Context.VariableDescMap)
		{
			if (SpvVariable* Var = this->Context.FindVar(VarId); Var && !Var->IsExternal())
			{
				PatchAppendExprFunc(Var->Type, VarDesc->TypeDesc);
			}
		}
		//Prevent spirv-cross from optimizing away functions that are never called.
		PatchAppendExprDummyFunc();

		auto& Context = this->Context;
		auto GetLineFromPackedHeader = [&](SpvOpFunctionCall* FuncCall) -> int32 {
			uint32 PackedHeader = *(uint32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[0]].Storage).Value.GetData();
			SpvDebuggerStateType StateType;
			uint32 Source, Line;
			UnpackDebugHeader(PackedHeader, StateType, Source, Line);
			return (int32)Line;
		};

		auto GetStateTypeFromPackedHeader = [&](SpvOpFunctionCall* FuncCall) -> SpvDebuggerStateType {
			uint32 PackedHeader = *(uint32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[0]].Storage).Value.GetData();
			SpvDebuggerStateType StateType;
			uint32 Source, Line;
			UnpackDebugHeader(PackedHeader, StateType, Source, Line);
			return StateType;
		};

		//Find a suitable location to insert AppendExprDummy.
		//StopDebugState is guaranteed to be a GPU-visible state (CPU-only states
		//like ScopeChange and FuncCallAfterReturn are resolved to the next GPU
		//state by EvaluateExpression before reaching here).
		TContext& ExprContext = static_cast<TContext&>(Context);

		int32 TargetLine = std::visit([](auto&& S) -> int32 {
			if constexpr (requires { S.Line; }) return S.Line;
			return 0;
		}, ExprContext.StopDebugState);

		bool IsVarChangeState = std::holds_alternative<SpvDebugState_VarChange>(ExprContext.StopDebugState);
		bool IsFuncCallState = std::holds_alternative<SpvDebugState_FuncCall>(ExprContext.StopDebugState);
		bool IsTagState = std::holds_alternative<SpvDebugState_Tag>(ExprContext.StopDebugState);
		bool IsAccessState = std::holds_alternative<SpvDebugState_Access>(ExprContext.StopDebugState);

		for (const auto& Inst : Patcher.GetPathcedInsts())
		{
			SpvOpFunctionCall* FuncCall = dynamic_cast<SpvOpFunctionCall*>(Inst.Get());
			if (!FuncCall) continue;

			int32 Line = GetLineFromPackedHeader(FuncCall);
			if (Line != TargetLine) continue;

			if (IsVarChangeState && this->AppendVarFuncIds.FindKey(FuncCall->GetFunction()))
			{
				const SpvOpStore* const* Store = this->AppendVarCallToStore.Find(FuncCall);
				AppendExprDummy([&] { return Store ? (*Store)->GetWordOffset().value() : FuncCall->GetWordOffset().value(); });
				break;
			}
			else if (IsFuncCallState && FuncCall->GetFunction() == this->AppendCallFuncId)
			{
				AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
				break;
			}
			else if (IsTagState && FuncCall->GetFunction() == this->AppendTagFuncId)
			{
				const auto& State = std::get<SpvDebugState_Tag>(ExprContext.StopDebugState);
				SpvDebuggerStateType StateType = GetStateTypeFromPackedHeader(FuncCall);
				if ((StateType == SpvDebuggerStateType::Condition && State.bCondition) ||
					(StateType == SpvDebuggerStateType::Kill && State.bKill) ||
					(StateType == SpvDebuggerStateType::Return && State.bReturn))
				{
					AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
					break;
				}
			}
			else if (IsAccessState && this->AppendAccessFuncIds.FindKey(FuncCall->GetFunction()))
			{
				const auto& State = std::get<SpvDebugState_Access>(ExprContext.StopDebugState);
				SpvId VarId = *(SpvId*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
				if (State.VarId == VarId)
				{
					AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
					break;
				}
			}
			else if (!IsVarChangeState && !IsFuncCallState && !IsTagState && !IsAccessState
				&& this->AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
			{
				AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
				break;
			}
		}
	}

	template<typename TBase, typename TContext>
	void SpvExprDebuggerVisitorImpl<TBase, TContext>::PatchAppendExprDummyFunc()
	{
		auto& Patcher = this->Patcher;
		AppendExprDummyFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendExprDummyFuncId, "_AppendExprDummy_"));
		TArray<TUniquePtr<SpvInstruction>> AppendExprDummyFuncInsts;
		{
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendExprDummyFuncId);
			AppendExprDummyFuncInsts.Add(MoveTemp(FuncOp));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendExprDummyFuncInsts.Add(MoveTemp(LabelOp));

			for (const auto& [_, Id] : AppendExprFuncIds)
			{
				auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, Id, TArray<SpvId>{DebugStateNum});
				FuncCallOp->SetId(Patcher.NewId());
				AppendExprDummyFuncInsts.Add(MoveTemp(FuncCallOp));
			}

			AppendExprDummyFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendExprDummyFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendExprDummyFuncInsts));
	}

	template<typename TBase, typename TContext>
	void SpvExprDebuggerVisitorImpl<TBase, TContext>::AppendExprDummy(const TFunction<int32()>& OffsetEval)
	{
		auto& Patcher = this->Patcher;
		TArray<TUniquePtr<SpvInstruction>> AppendExprDummyInsts;
		{
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());

			AppendExprDummyInsts.Add(MakeUnique<SpvOpStore>(DoAppendExpr, Patcher.FindOrAddConstant(1u)));

			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendExprDummyFuncId);
			FuncCallOp->SetId(Patcher.NewId());
			AppendExprDummyInsts.Add(MoveTemp(FuncCallOp));

			AppendExprDummyInsts.Add(MakeUnique<SpvOpStore>(DoAppendExpr, Patcher.FindOrAddConstant(0u)));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendExprDummyInsts));
	}

	template<typename TBase, typename TContext>
	bool SpvExprDebuggerVisitorImpl<TBase, TContext>::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		PatchInvocationGate(InstList);

		auto& Patcher = this->Patcher;
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

		SpvId LoadedDoAppendExpr = Patcher.NewId();
		auto LoadedDoAppendExprOp = MakeUnique<SpvOpLoad>(UIntType, DoAppendExpr);
		LoadedDoAppendExprOp->SetId(LoadedDoAppendExpr);
		InstList.Add(MoveTemp(LoadedDoAppendExprOp));

		SpvId IEqual = Patcher.NewId();
		auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, LoadedDoAppendExpr, Patcher.FindOrAddConstant(0u));
		IEqualOp->SetId(IEqual);
		InstList.Add(MoveTemp(IEqualOp));

		auto TrueLabel = Patcher.NewId();
		auto FalseLabel = Patcher.NewId();
		InstList.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
		InstList.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));
		auto FalseLabelOp = MakeUnique<SpvOpLabel>();
		FalseLabelOp->SetId(FalseLabel);
		InstList.Add(MoveTemp(FalseLabelOp));
		InstList.Add(MakeUnique<SpvOpReturn>());
		auto TrueLabelOp = MakeUnique<SpvOpLabel>();
		TrueLabelOp->SetId(TrueLabel);
		InstList.Add(MoveTemp(TrueLabelOp));

		SpvId LoadedDebugStateNum = Patcher.NewId();
		auto LoadedDebugStateNumOp = MakeUnique<SpvOpLoad>(UIntType, DebugStateNum);
		LoadedDebugStateNumOp->SetId(LoadedDebugStateNum);
		InstList.Add(MoveTemp(LoadedDebugStateNumOp));

		SpvId NewDebugStateNum = Patcher.NewId();
		auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedDebugStateNum, Patcher.FindOrAddConstant(1u));
		AddOp->SetId(NewDebugStateNum);
		InstList.Add(MoveTemp(AddOp));

		InstList.Add(MakeUnique<SpvOpStore>(DebugStateNum, NewDebugStateNum));

		return false;
	}

	template<typename TBase, typename TContext>
	void SpvExprDebuggerVisitorImpl<TBase, TContext>::PatchBaseTypeAppendExprFunc()
	{
		auto& Patcher = this->Patcher;
		auto& Context = this->Context;
		SpvId DebugExtSet = *Context.ExtSets.FindKey(SpvExtSet::NonSemanticShaderDebugInfo100);
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());

		auto PatchBasic = [&](SpvId ScalarType, const char* DebugName, SpvDebugBasicTypeEncoding Encoding)
		{
			SpvId TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeBasic>(VoidType, DebugExtSet,
				Patcher.FindOrAddDebugStr(DebugName), Patcher.FindOrAddConstant(32u),
				Patcher.FindOrAddConstant((uint32)Encoding), Patcher.FindOrAddConstant(0u)));
			PatchAppendExprFunc(Context.Types[ScalarType].Get(), Context.TypeDescs[TypeDesc].Get());

			for (uint32 N = 2; N <= 4; ++N)
			{
				SpvId VecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(ScalarType, N));
				SpvId VecTypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
					TypeDesc, Patcher.FindOrAddConstant(N)));
				PatchAppendExprFunc(Context.Types[VecType].Get(), Context.TypeDescs[VecTypeDesc].Get());
			}
		};

		PatchBasic(Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32)), "float", SpvDebugBasicTypeEncoding::Float);
		PatchBasic(Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0)), "uint", SpvDebugBasicTypeEncoding::Unsigned);
		PatchBasic(Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>()), "bool", SpvDebugBasicTypeEncoding::Boolean);
		PatchBasic(Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1)), "int", SpvDebugBasicTypeEncoding::Signed);
	}

	template<typename TBase, typename TContext>
	void SpvExprDebuggerVisitorImpl<TBase, TContext>::PatchAppendExprFunc(const SpvType* Type, const SpvTypeDesc* TypeDesc)
	{
		if (!TypeDesc || AppendExprFuncIds.Contains(Type))
		{
			return;
		}

		//Avoid hlsl overload resolution ambiguity between float2x2 and float4.
		//TODO glsl
		if (const SpvMatrixType* MatrixType = dynamic_cast<const SpvMatrixType*>(Type);
			MatrixType && MatrixType->ElementCount == 2 && MatrixType->ElementType->ElementCount == 2)
		{
			return;
		}

		auto& Patcher = this->Patcher;
		auto& Context = this->Context;
		TContext& ExprContext = static_cast<TContext&>(Context);

		SpvId AppendExprFuncId = Patcher.NewId();
		FString FuncName = "_AppendExpr_";
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendExprFuncId, FuncName));
		TArray<TUniquePtr<SpvInstruction>> AppendExprFuncInsts;
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			TArray<SpvId> ParamterTypes;
			ParamterTypes.Add(Type->GetId());

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendExprFuncId);
			AppendExprFuncInsts.Add(MoveTemp(FuncOp));

			SpvId ValueParam = Patcher.NewId();
			auto ValueParamOp = MakeUnique<SpvOpFunctionParameter>(Type->GetId());
			ValueParamOp->SetId(ValueParam);
			AppendExprFuncInsts.Add(MoveTemp(ValueParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ValueParam, "_ExprValue_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendExprFuncInsts.Add(MoveTemp(LabelOp));

			PatchInvocationGate(AppendExprFuncInsts);

			SpvId LoadedDebugStateNum = Patcher.NewId();
			auto LoadedDebugStateNumOp = MakeUnique<SpvOpLoad>(UIntType, DebugStateNum);
			LoadedDebugStateNumOp->SetId(LoadedDebugStateNum);
			AppendExprFuncInsts.Add(MoveTemp(LoadedDebugStateNumOp));

			SpvId IEqual = Patcher.NewId();
			auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, LoadedDebugStateNum,
				Patcher.FindOrAddConstant((uint32)ExprContext.StopDebugStateIndex));
			IEqualOp->SetId(IEqual);
			AppendExprFuncInsts.Add(MoveTemp(IEqualOp));

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			AppendExprFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
			AppendExprFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));
			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			AppendExprFuncInsts.Add(MoveTemp(FalseLabelOp));
			AppendExprFuncInsts.Add(MakeUnique<SpvOpReturn>());
			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			AppendExprFuncInsts.Add(MoveTemp(TrueLabelOp));

			this->PatchToDebugger(Patcher.FindOrAddConstant(Context.GetTypeDescId(TypeDesc).GetValue()), UIntType, AppendExprFuncInsts);
			this->PatchToDebugger(Patcher.FindOrAddConstant((uint32)GetTypeByteSize(Type)), UIntType, AppendExprFuncInsts);
			this->PatchToDebugger(ValueParam, Type->GetId(), AppendExprFuncInsts);

			AppendExprFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendExprFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendExprFuncInsts));
		AppendExprFuncIds.Add(Type, AppendExprFuncId);

		if (TypeDesc->GetKind() == SpvTypeDescKind::Composite && Type->GetKind() == SpvTypeKind::Struct)
		{
			const SpvStructType* StructType = static_cast<const SpvStructType*>(Type);
			const SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<const SpvCompositeTypeDesc*>(TypeDesc);
			const TArray<SpvTypeDesc*>& MemberTypeDescs = CompositeTypeDesc->GetMemberTypeDescs();
			for (int Index = 0; Index < MemberTypeDescs.Num(); Index++)
			{
				PatchAppendExprFunc(StructType->MemberTypes[Index], MemberTypeDescs[Index]);
			}
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Array && Type->GetKind() == SpvTypeKind::Array)
		{
			const SpvArrayType* ArrayType = static_cast<const SpvArrayType*>(Type);
			const SpvArrayTypeDesc* ArrayTypeDesc = static_cast<const SpvArrayTypeDesc*>(TypeDesc);
			PatchAppendExprFunc(ArrayType->ElementType, ArrayTypeDesc->GetBaseTypeDesc());
		}
	}
}
