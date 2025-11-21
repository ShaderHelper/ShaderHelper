#include "CommonHeader.h"
#include "SpirvExprDebugger.h"

namespace FW
{
	void SpvPixelExprDebuggerVisitor::Visit(const SpvDebugDeclare* Inst)
	{
		SpvDebuggerVisitor::Visit(Inst);

		SpvId VarId = Inst->GetVariable();
		SpvVariable* Var = Context.FindVar(VarId);

		//Insert an extra load instruction to prevent spirv-cross from optimizing away the variable.
		SpvId LoadedVar = Patcher.NewId();
		auto LoadedVarOp = MakeUnique<SpvOpLoad>(Var->Type->GetId(), VarId);
		LoadedVarOp->SetId(LoadedVar);
		Patcher.AddInstruction(Inst->GetWordOffset().value(), MoveTemp(LoadedVarOp));

	}

	void SpvPixelExprDebuggerVisitor::ParseInternal()
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));

		DebugStateNum = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebugStateNum);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebugStateNum, "_DebugStateNum_"));
		}
		DoAppendExpr = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DoAppendExpr);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DoAppendExpr, "_DoAppendExpr_"));
		}

		SpvPixelDebuggerVisitor::ParseInternal();

		PatchBaseTypeAppendExprFunc();
		for (const auto& [VarId, VarDesc] : Context.VariableDescMap)
		{
			if (SpvVariable* Var = Context.FindVar(VarId) ; Var && !Var->IsExternal())
			{
				PatchAppendExprFunc(Var->Type, VarDesc->TypeDesc);
			}
		}
		//Prevent spirv-cross from optimizing away functions that are never called.
		PatchAppendExprDummyFunc();

		//Find a suitable location to insert AppendExprDummy
		SpvPixelExprDebuggerContext& ExprContext = static_cast<SpvPixelExprDebuggerContext&>(Context);
		for(const auto& Inst : Patcher.GetPathcedInsts())
		{
			if (SpvOpFunctionCall * FuncCall = dynamic_cast<SpvOpFunctionCall*>(Inst.Get()))
			{
				if (std::holds_alternative<SpvDebugState_VarChange>(ExprContext.StopDebugState) && AppendVarFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_VarChange>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return AppendVarCallToStore[FuncCall]->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_FuncCall>(ExprContext.StopDebugState) && FuncCall->GetFunction() == AppendCallFuncId)
				{
					const auto& State = std::get<SpvDebugState_FuncCall>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_Tag>(ExprContext.StopDebugState) && FuncCall->GetFunction() == AppendTagFuncId)
				{
					const auto& State = std::get<SpvDebugState_Tag>(ExprContext.StopDebugState);
					SpvDebuggerStateType StateType = *(SpvDebuggerStateType*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[0]].Storage).Value.GetData();
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (((StateType == SpvDebuggerStateType::Condition && State.bCondition) ||
						(StateType == SpvDebuggerStateType::FuncCallAfterReturn && State.bFuncCallAfterReturn) ||
						(StateType == SpvDebuggerStateType::Kill && State.bKill) ||
						(StateType == SpvDebuggerStateType::Return && State.bReturn)) &&
						State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				//Stopped due to UBSan.
				else if (std::holds_alternative<SpvDebugState_Access>(ExprContext.StopDebugState) && AppendAccessFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_Access>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_Normalize>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_Normalize>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_SmoothStep>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_SmoothStep>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_Pow>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_Pow>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_Clamp>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_Clamp>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_Div>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_Div>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_ConvertF>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_ConvertF>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
				else if (std::holds_alternative<SpvDebugState_Remainder>(ExprContext.StopDebugState) && AppendMathFuncIds.FindKey(FuncCall->GetFunction()))
				{
					const auto& State = std::get<SpvDebugState_Remainder>(ExprContext.StopDebugState);
					int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[FuncCall->GetArguments()[1]].Storage).Value.GetData();
					if (State.Line == Line)
					{
						AppendExprDummy([&] { return FuncCall->GetWordOffset().value(); });
						break;
					}
				}
			}
		}
	
	}

	void SpvPixelExprDebuggerVisitor::PatchAppendExprDummyFunc()
	{
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

			for (const auto& [_,Id] : AppendExprFuncIds)
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

	void SpvPixelExprDebuggerVisitor::AppendExprDummy(const TFunction<int32()>& OffsetEval)
	{
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

	void SpvPixelExprDebuggerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvPixelDebuggerVisitor::PatchActiveCondition(InstList);

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

		SpvId LoadedDoAppendExpr = Patcher.NewId();
		auto LoadedDoAppendExprOp = MakeUnique<SpvOpLoad>(UIntType, DoAppendExpr);
		LoadedDoAppendExprOp->SetId(LoadedDoAppendExpr);
		InstList.Add(MoveTemp(LoadedDoAppendExprOp));

		SpvId IEqual = Patcher.NewId();;
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
		InstList.Add(MakeUnique<SpvOpReturn>());

		auto LabelOp = MakeUnique<SpvOpLabel>();
		LabelOp->SetId(Patcher.NewId());
		InstList.Add(MoveTemp(LabelOp));
	}

	void SpvPixelExprDebuggerVisitor::PatchBaseTypeAppendExprFunc()
	{
		SpvId DebugExtSet = *Context.ExtSets.FindKey(SpvExtSet::NonSemanticShaderDebugInfo100);
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());

		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId FloatTypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeBasic>(VoidType, DebugExtSet, 
			Patcher.FindOrAddDebugStr("float"), Patcher.FindOrAddConstant(32u), 
			Patcher.FindOrAddConstant((uint32)SpvDebugBasicTypeEncoding::Float), Patcher.FindOrAddConstant(0u)));
		PatchAppendExprFunc(Context.Types[FloatType].Get(), Context.TypeDescs[FloatTypeDesc].Get());

		SpvId Float2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 2));
		SpvId Float2TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			FloatTypeDesc, Patcher.FindOrAddConstant(2u)));
		PatchAppendExprFunc(Context.Types[Float2Type].Get(), Context.TypeDescs[Float2TypeDesc].Get());

		SpvId Float3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 3));
		SpvId Float3TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			FloatTypeDesc, Patcher.FindOrAddConstant(3u)));
		PatchAppendExprFunc(Context.Types[Float3Type].Get(), Context.TypeDescs[Float3TypeDesc].Get());

		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));
		SpvId Float4TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			FloatTypeDesc, Patcher.FindOrAddConstant(4u)));
		PatchAppendExprFunc(Context.Types[Float4Type].Get(), Context.TypeDescs[Float4TypeDesc].Get());

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntTypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeBasic>(VoidType, DebugExtSet,
			Patcher.FindOrAddDebugStr("uint"), Patcher.FindOrAddConstant(32u), 
			Patcher.FindOrAddConstant((uint32)SpvDebugBasicTypeEncoding::Unsigned), Patcher.FindOrAddConstant(0u)));
		PatchAppendExprFunc(Context.Types[UIntType].Get(), Context.TypeDescs[UIntTypeDesc].Get());

		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
		SpvId UInt2TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			UIntTypeDesc, Patcher.FindOrAddConstant(2u)));
		PatchAppendExprFunc(Context.Types[UInt2Type].Get(), Context.TypeDescs[UInt2TypeDesc].Get());

		SpvId UInt3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 3));
		SpvId UInt3TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			UIntTypeDesc, Patcher.FindOrAddConstant(3u)));
		PatchAppendExprFunc(Context.Types[UInt3Type].Get(), Context.TypeDescs[UInt3TypeDesc].Get());

		SpvId UInt4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 4));
		SpvId UInt4TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			UIntTypeDesc, Patcher.FindOrAddConstant(4u)));
		PatchAppendExprFunc(Context.Types[UInt4Type].Get(), Context.TypeDescs[UInt4TypeDesc].Get());

		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId BoolTypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeBasic>(VoidType, DebugExtSet,
			Patcher.FindOrAddDebugStr("bool"), Patcher.FindOrAddConstant(32u), 
			Patcher.FindOrAddConstant((uint32)SpvDebugBasicTypeEncoding::Boolean), Patcher.FindOrAddConstant(0u)));
		PatchAppendExprFunc(Context.Types[BoolType].Get(), Context.TypeDescs[BoolTypeDesc].Get());

		SpvId Bool2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 2));
		SpvId Bool2TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			BoolTypeDesc, Patcher.FindOrAddConstant(2u)));
		PatchAppendExprFunc(Context.Types[Bool2Type].Get(), Context.TypeDescs[Bool2TypeDesc].Get());

		SpvId Bool3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 3));
		SpvId Bool3TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			BoolTypeDesc, Patcher.FindOrAddConstant(3u)));
		PatchAppendExprFunc(Context.Types[Bool3Type].Get(), Context.TypeDescs[Bool3TypeDesc].Get());

		SpvId Bool4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 4));
		SpvId Bool4TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			BoolTypeDesc, Patcher.FindOrAddConstant(4u)));
		PatchAppendExprFunc(Context.Types[Bool4Type].Get(), Context.TypeDescs[Bool4TypeDesc].Get());

		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
		SpvId IntTypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeBasic>(VoidType, DebugExtSet,
			Patcher.FindOrAddDebugStr("int"), Patcher.FindOrAddConstant(32u), 
			Patcher.FindOrAddConstant((uint32)SpvDebugBasicTypeEncoding::Signed), Patcher.FindOrAddConstant(0u)));
		PatchAppendExprFunc(Context.Types[IntType].Get(), Context.TypeDescs[IntTypeDesc].Get());

		SpvId Int2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(IntType, 2));
		SpvId Int2TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			IntTypeDesc, Patcher.FindOrAddConstant(2u)));
		PatchAppendExprFunc(Context.Types[Int2Type].Get(), Context.TypeDescs[Int2TypeDesc].Get());

		SpvId Int3Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(IntType, 3));
		SpvId Int3TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			IntTypeDesc, Patcher.FindOrAddConstant(3u)));
		PatchAppendExprFunc(Context.Types[Int3Type].Get(), Context.TypeDescs[Int3TypeDesc].Get());

		SpvId Int4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(IntType, 4));
		SpvId Int4TypeDesc = Patcher.FindOrAddTypeDesc(MakeUnique<SpvDebugTypeVector>(VoidType, DebugExtSet,
			IntTypeDesc, Patcher.FindOrAddConstant(4u)));
		PatchAppendExprFunc(Context.Types[Int4Type].Get(), Context.TypeDescs[Int4TypeDesc].Get());

	}

	void SpvPixelExprDebuggerVisitor::PatchAppendExprFunc(const SpvType* Type, const SpvTypeDesc* TypeDesc)
	{
		if (!TypeDesc || AppendExprFuncIds.Contains(Type))
		{
			return;
		}

		//Avoid hlsl overload resolution ambiguity between float2x2 and float4
		//TODO glsl
		if (const SpvMatrixType* MatrixType = dynamic_cast<const SpvMatrixType*>(Type);
			MatrixType && MatrixType->ElementCount == 2 && MatrixType->ElementType->ElementCount == 2)
		{
			return;
		}

		SpvPixelExprDebuggerContext& ExprContext = static_cast<SpvPixelExprDebuggerContext&>(Context);

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

			SpvPixelDebuggerVisitor::PatchActiveCondition(AppendExprFuncInsts);

			SpvId LoadedDebugStateNum = Patcher.NewId();
			auto LoadedDebugStateNumOp = MakeUnique<SpvOpLoad>(UIntType, DebugStateNum);
			LoadedDebugStateNumOp->SetId(LoadedDebugStateNum);
			AppendExprFuncInsts.Add(MoveTemp(LoadedDebugStateNumOp));

			SpvId IEqual = Patcher.NewId();;
			auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, LoadedDebugStateNum, Patcher.FindOrAddConstant((uint32)ExprContext.StopDebugStateIndex));
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

			PatchToDebugger(Patcher.FindOrAddConstant(Context.GetTypeDescId(TypeDesc).GetValue()), UIntType, AppendExprFuncInsts);
			PatchToDebugger(Patcher.FindOrAddConstant((uint32)GetTypeByteSize(Type)), UIntType, AppendExprFuncInsts);
			PatchToDebugger(ValueParam, Type->GetId(), AppendExprFuncInsts);

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
