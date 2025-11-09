#include "CommonHeader.h"
#include "ShaderDebugger.h"
#include "ShaderConductor.hpp"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "RenderResource/Shader/Shader.h"

using namespace FW;

namespace SH
{
	TArray<ExpressionNodePtr> AppendVarChildNodes(SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, const TArray<SpvVariableChange::DirtyRange>& DirtyRanges, const TArray<uint8>& Value, int32 InOffset)
	{
		TArray<ExpressionNodePtr> Nodes;
		int32 Offset = InOffset;
		if (TypeDesc->GetKind() == SpvTypeDescKind::Member)
		{
			SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(TypeDesc);
			int32 MemberByteSize = GetTypeByteSize(MemberTypeDesc);
			FString ValueStr = GetValueStr(Value, MemberTypeDesc->GetTypeDesc(), InitializedRanges, Offset);
			auto Data = MakeShared<ExpressionNode>(MemberTypeDesc->GetName(), MoveTemp(ValueStr), GetTypeDescStr(MemberTypeDesc->GetTypeDesc()));
			if (MemberTypeDesc->GetTypeDesc()->GetKind() == SpvTypeDescKind::Composite)
			{
				Data->Children = AppendVarChildNodes(MemberTypeDesc->GetTypeDesc(), InitializedRanges, DirtyRanges, Value, Offset);
			}
			for (const auto& Range : DirtyRanges)
			{
				int32 RangeStart = Range.OffsetBytes;
				int32 RangeEnd = Range.OffsetBytes + Range.ByteSize;
				if ((RangeStart >= InOffset && RangeStart < InOffset + MemberByteSize) ||
					(RangeEnd > InOffset && RangeEnd <= InOffset + MemberByteSize) ||
					(RangeStart < InOffset && RangeEnd > InOffset + MemberByteSize))
				{
					Data->Dirty = true;
					break;
				}
			}
			Nodes.Add(MoveTemp(Data));
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(TypeDesc);
			for (SpvTypeDesc* MemberTypeDesc : CompositeTypeDesc->GetMemberTypeDescs())
			{
				Nodes.Append(AppendVarChildNodes(MemberTypeDesc, InitializedRanges, DirtyRanges, Value, Offset));
				if (MemberTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					Offset += GetTypeByteSize(MemberTypeDesc);
				}
			}
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
		{
			SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<SpvMatrixTypeDesc*>(TypeDesc);
			SpvVectorTypeDesc* ElementTypeDesc = MatrixTypeDesc->GetVectorTypeDesc();
			int32 VectorCount = MatrixTypeDesc->GetVectorCount();
			int32 ElementTypeSize = GetTypeByteSize(ElementTypeDesc);
			for (int32 Index = 0; Index < VectorCount; Index++)
			{
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset);
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				auto Data = MakeShared<ExpressionNode>(MemberName, MoveTemp(ValueStr), GetTypeDescStr(ElementTypeDesc));
				for (const auto& Range : DirtyRanges)
				{
					int32 RangeStart = Range.OffsetBytes;
					int32 RangeEnd = Range.OffsetBytes + Range.ByteSize;
					if ((RangeStart >= Offset && RangeStart < Offset + ElementTypeSize) ||
						(RangeEnd > Offset && RangeEnd <= Offset + ElementTypeSize) ||
						(RangeStart < Offset && RangeEnd > Offset + ElementTypeSize))
					{
						Data->Dirty = true;
						break;
					}
				}
				Offset += ElementTypeSize;
				Nodes.Add(MoveTemp(Data));
			}
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(TypeDesc);
			SpvTypeDesc* BaseTypeDesc = ArrayTypeDesc->GetBaseTypeDesc();
			int32 BaseTypeSize = GetTypeByteSize(BaseTypeDesc);
			int32 CompCount = ArrayTypeDesc->GetCompCounts()[0];
			SpvTypeDesc* ElementTypeDesc = BaseTypeDesc;
			int32 ElementTypeSize = BaseTypeSize;
			TUniquePtr<SpvArrayTypeDesc> SubArrayTypeDesc = ArrayTypeDesc->GetCompCounts().Num() > 1 ? MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, TArray{ ArrayTypeDesc->GetCompCounts().GetData() + 1, ArrayTypeDesc->GetCompCounts().Num() - 1 }) : nullptr;
			if (SubArrayTypeDesc)
			{
				ElementTypeDesc = SubArrayTypeDesc.Get();
				ElementTypeSize *= SubArrayTypeDesc->GetElementNum();
			}
			for (int32 Index = 0; Index < CompCount; Index++)
			{
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset);
				auto Data = MakeShared<ExpressionNode>(MemberName, MoveTemp(ValueStr), GetTypeDescStr(ElementTypeDesc));
				Data->Children = AppendVarChildNodes(ElementTypeDesc, InitializedRanges, DirtyRanges, Value, Offset);
				for (const auto& Range : DirtyRanges)
				{
					int32 RangeStart = Range.OffsetBytes;
					int32 RangeEnd = Range.OffsetBytes + Range.ByteSize;
					if ((RangeStart >= Offset && RangeStart < Offset + ElementTypeSize) ||
						(RangeEnd > Offset && RangeEnd <= Offset + ElementTypeSize) ||
						(RangeStart < Offset && RangeEnd > Offset + ElementTypeSize))
					{
						Data->Dirty = true;
						break;
					}
				}
				Nodes.Add(MoveTemp(Data));
				Offset += ElementTypeSize;
			}
		}
		return Nodes;
	}

	TArray<ExpressionNodePtr> AppendExprChildNodes(SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, const TArray<uint8>& Value, int32 InOffset)
	{
		TArray<ExpressionNodePtr> Nodes;
		int32 Offset = InOffset;
		if (TypeDesc->GetKind() == SpvTypeDescKind::Member)
		{
			SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(TypeDesc);
			int32 MemberByteSize = GetTypeByteSize(MemberTypeDesc);
			FString ValueStr = GetValueStr(Value, MemberTypeDesc->GetTypeDesc(), InitializedRanges, Offset);
			FString TypeName = GetTypeDescStr(MemberTypeDesc->GetTypeDesc());
			auto Data = MakeShared<ExpressionNode>(MemberTypeDesc->GetName(), ValueStr, TypeName);
			if (MemberTypeDesc->GetTypeDesc()->GetKind() == SpvTypeDescKind::Composite)
			{
				Data->Children = AppendExprChildNodes(MemberTypeDesc->GetTypeDesc(), InitializedRanges, Value, Offset);
			}
			Nodes.Add(MoveTemp(Data));
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(TypeDesc);
			for (SpvTypeDesc* MemberTypeDesc : CompositeTypeDesc->GetMemberTypeDescs())
			{
				Nodes.Append(AppendExprChildNodes(MemberTypeDesc, InitializedRanges, Value, Offset));
				if (MemberTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					Offset += GetTypeByteSize(MemberTypeDesc);
				}
			}
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
		{
			SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<SpvMatrixTypeDesc*>(TypeDesc);
			SpvVectorTypeDesc* ElementTypeDesc = MatrixTypeDesc->GetVectorTypeDesc();
			int32 VectorCount = MatrixTypeDesc->GetVectorCount();
			int32 ElementTypeSize = GetTypeByteSize(ElementTypeDesc);
			for (int32 Index = 0; Index < VectorCount; Index++)
			{
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset);
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				FString TypeName = GetTypeDescStr(ElementTypeDesc);
				auto Data = MakeShared<ExpressionNode>(MemberName, ValueStr, TypeName);
				Offset += ElementTypeSize;
				Nodes.Add(MoveTemp(Data));
			}
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(TypeDesc);
			SpvTypeDesc* BaseTypeDesc = ArrayTypeDesc->GetBaseTypeDesc();
			int32 BaseTypeSize = GetTypeByteSize(BaseTypeDesc);
			int32 CompCount = ArrayTypeDesc->GetCompCounts()[0];
			SpvTypeDesc* ElementTypeDesc = BaseTypeDesc;
			int32 ElementTypeSize = BaseTypeSize;
			TUniquePtr<SpvArrayTypeDesc> SubArrayTypeDesc = ArrayTypeDesc->GetCompCounts().Num() > 1 ? MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, TArray{ ArrayTypeDesc->GetCompCounts().GetData() + 1, ArrayTypeDesc->GetCompCounts().Num() - 1 }) : nullptr;
			if (SubArrayTypeDesc)
			{
				ElementTypeDesc = SubArrayTypeDesc.Get();
				ElementTypeSize *= SubArrayTypeDesc->GetElementNum();
			}
			for (int32 Index = 0; Index < CompCount; Index++)
			{
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset);
				FString TypeName = GetTypeDescStr(ElementTypeDesc);
				auto Data = MakeShared<ExpressionNode>(MemberName, ValueStr, TypeName);
				Data->Children = AppendExprChildNodes(ElementTypeDesc, InitializedRanges, Value, Offset);
				Nodes.Add(MoveTemp(Data));
				Offset += ElementTypeSize;
			}
		}
		return Nodes;
	}

	ExpressionNode ShaderDebugger::EvaluateExpression(const FString& InExpression) const
	{
		return{};
		ShaderAsset* ShaderAssetObj = ShaderEditor->GetShaderAsset();
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();

		FString ExpressionShader = ShaderAssetObj->GetShaderDesc(ShaderEditor->GetCurShaderSource()).Source;
		TArray<uint8> InputData;

		//Patch EntryPoint
		FString EntryPoint = ShaderAssetObj->Shader->GetEntryPoint();
		for (const ShaderFunc& Func : Funcs)
		{
			if (Func.Name == EntryPoint)
			{
				TArray<FTextRange> LineRanges;
				FTextRange::CalculateLineRangesFromString(ExpressionShader, LineRanges);
				int32 StartPos = LineRanges[Func.Start.x - 1].BeginIndex + Func.Start.y - 1;
				int32 EndPos = LineRanges[Func.End.x - 1].BeginIndex + Func.End.y - 1;

				ExpressionShader.RemoveAt(StartPos, EndPos - StartPos);

				//Append bindings
				TArray<FString> LocalVars;
				FString LocalVarInitializations;
				FString VisibleLocalVarBindings = "struct __Expression_Vars_Set {";
				for (const auto& [VarId, VarDesc] : SortedVariableDescs)
				{
					if (SpvVariable* Var = DebuggerContext->FindVar(VarId))
					{
						if (!Var->IsExternal() && VarDesc)
						{
							bool VisibleScope = VarDesc->Parent->Contains(CallStackScope) || (DebugStates[CurDebugStateIndex].bReturn && Scope->GetKind() == SpvScopeKind::Function && CallStackScope == VarDesc->Parent->GetParent());
							bool VisibleLine = VarDesc->Line < StopLineNumber + ExtraLineNum && VarDesc->Line > ExtraLineNum;
							if (VisibleScope && VisibleLine)
							{
								//If there are variables with the same name, only the one in the most recent scope is evaluated.
								if (LocalVars.Contains(VarDesc->Name))
								{
									continue;
								}
								FString Declaration;
								if (VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Array)
								{
									SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(VarDesc->TypeDesc);
									FString BasicTypeStr = GetTypeDescStr(ArrayTypeDesc->GetBaseTypeDesc());
									FString DimStr = GetTypeDescStr(VarDesc->TypeDesc);
									DimStr.RemoveFromStart(BasicTypeStr);
									Declaration = BasicTypeStr + " " + VarDesc->Name + DimStr + ";";
									LocalVarInitializations += Declaration;
									LocalVarInitializations += VarDesc->Name + "= __Expression_Vars[0]." + VarDesc->Name + ";\n";
								}
								else
								{
									Declaration = GetTypeDescStr(VarDesc->TypeDesc) + " " + VarDesc->Name + ";";
									LocalVarInitializations += GetTypeDescStr(VarDesc->TypeDesc) + " " + VarDesc->Name + "= __Expression_Vars[0]." + VarDesc->Name + ";\n";
								}
								VisibleLocalVarBindings += Declaration;
								LocalVars.Add(VarDesc->Name);
								const TArray<uint8>& Value = std::get<SpvObject::Internal>(Var->Storage).Value;
								InputData.Append(Value);
							}
						}
					}
				}
				VisibleLocalVarBindings += "};\n";
				VisibleLocalVarBindings += "StructuredBuffer<__Expression_Vars_Set> __Expression_Vars : register(t114514);\n";

				//Append the EntryPoint
				FString EntryPointFunc = FString::Printf(TEXT("\nvoid %s() {\n"), *EntryPoint);
				EntryPointFunc += MoveTemp(LocalVarInitializations);
				EntryPointFunc += FString::Printf(TEXT("__Expression_Output(%s);\n"), *InExpression);
				EntryPointFunc += "}\n";
				ExpressionShader += VisibleLocalVarBindings + EntryPointFunc;
				break;
			}
		}

		static const FString OutputFunc =
			R"(template<typename T>
void __Expression_Output(T __Expression_Result) {}
)";
		ExpressionShader = OutputFunc + ExpressionShader;

		auto ShaderDesc = GpuShaderSourceDesc{
			.Name = "EvaluateExpression",
			.Source = MoveTemp(ExpressionShader),
			.Type = ShaderAssetObj->Shader->GetShaderType(),
			.EntryPoint = EntryPoint
		};
		TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(ShaderDesc);
		Shader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo, ExtraArgs);
		if (ErrorInfo.IsEmpty())
		{
			TRefCountPtr<GpuBuffer> LocalVarsInput = GGpuRhi->CreateBuffer({
				.ByteSize = (uint32)InputData.Num(),
				.Usage = GpuBufferUsage::Structured,
				.InitialData = InputData,
				.StructuredInit = {
					.Stride = (uint32)InputData.Num()
				}
				});

			if (Shader->GetShaderType() == ShaderType::PixelShader)
			{
				/*if(!ExprContext.HasSideEffect)
				{
					FText TypeName = LOCALIZATION("InvalidExpr");
					if(ExprContext.ResultTypeDesc)
					{
						TypeName = FText::FromString(GetTypeDescStr(ExprContext.ResultTypeDesc));

						TArray<Vector2i> ResultRange;
						ResultRange.Add({ 0, ExprContext.ResultValue.Num() });
						FString ValueStr = GetValueStr(ExprContext.ResultValue, ExprContext.ResultTypeDesc, ResultRange, 0);

						TArray<TSharedPtr<ExpressionNode>> Children;
						if (ExprContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Composite || ExprContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Array
							|| ExprContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
						{
							Children = AppendExprChildNodes(ExprContext.ResultTypeDesc, ResultRange, ExprContext.ResultValue, 0);
						}

						return { .Expr = InExpression, .ValueStr = ValueStr,
							.TypeName = TypeName.ToString(), .Children = MoveTemp(Children)};
					}

				}
				else*/
				{
					return { .Expr = InExpression, .ValueStr = LOCALIZATION("SideEffectExpr").ToString() };
				}
			}
		}

		return { .Expr = InExpression, .ValueStr = LOCALIZATION("InvalidExpr").ToString() };
	}

	void ShaderDebugger::ShowDebuggerResult() const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerCallStackView* DebuggerCallStackView = ShEditor->GetDebuggerCallStackView();
		ShaderAsset* ShaderAssetObj = ShaderEditor->GetShaderAsset();
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
		TArray<CallStackDataPtr> CallStackDatas;

		SpvFunctionDesc* FuncDesc = GetFunctionDesc(Scope);
		FString Location = FString::Printf(TEXT("%s (Line %d)"), *ShaderAssetObj->GetFileName(), StopLineNumber);
		CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc, Funcs), MoveTemp(Location)));
		for (int i = CallStack.Num() - 1; i >= 0; i--)
		{
			SpvFunctionDesc* FuncDesc = GetFunctionDesc(CallStack[i].Key);
			int JumpLineNumber = CallStack[i].Value - ExtraLineNum;
			if (JumpLineNumber > 0)
			{
				FString Location = FString::Printf(TEXT("%s (Line %d)"), *ShaderAssetObj->GetFileName(), JumpLineNumber);
				CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc, Funcs), MoveTemp(Location)));
			}
		}
		DebuggerCallStackView->SetCallStackDatas(CallStackDatas);

		/*ShowDeuggerVariable(Scope);

		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();
		DebuggerWatchView->OnWatch = [this](const FString& Expression) { return EvaluateExpression(Expression); };
		DebuggerWatchView->Refresh();*/
	}

	void ShaderDebugger::ShowDeuggerVariable(SpvLexicalScope* InScope) const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();

		TArray<ExpressionNodePtr> LocalVarNodeDatas;
		TArray<ExpressionNodePtr> GlobalVarNodeDatas;

		if (CurReturnObject && Scope == InScope)
		{
			SpvTypeDesc* ReturnTypeDesc = std::get<SpvTypeDesc*>(GetFunctionDesc(Scope)->GetFuncTypeDesc()->GetReturnType());
			const TArray<uint8>& Value = std::get<SpvObject::Internal>(CurReturnObject.value().Storage).Value;
			FString VarName = GetFunctionDesc(Scope)->GetName() + LOCALIZATION("ReturnTip").ToString();
			FString TypeName = GetTypeDescStr(ReturnTypeDesc);
			FString ValueStr = GetValueStr(Value, ReturnTypeDesc, TArray{ Vector2i{0, Value.Num()} }, 0);

			auto Data = MakeShared<ExpressionNode>(VarName, ValueStr, TypeName);
			LocalVarNodeDatas.Add(MoveTemp(Data));
		}

		for (const auto& [VarId, VarDesc] : SortedVariableDescs)
		{
			if (SpvVariable* Var = DebuggerContext->FindVar(VarId))
			{
				if (!Var->IsExternal())
				{
					bool VisibleScope = VarDesc->Parent->Contains(InScope) || (DebugStates[CurDebugStateIndex].bReturn && InScope->GetKind() == SpvScopeKind::Function && InScope == VarDesc->Parent->GetParent());
					bool VisibleLine = VarDesc->Line <= StopLineNumber + ExtraLineNum && VarDesc->Line > ExtraLineNum;
					if (VisibleScope && VisibleLine)
					{
						FString VarName = VarDesc->Name;
						//If there are variables with the same name, only the one in the most recent scope is shown.
						if (LocalVarNodeDatas.ContainsByPredicate([&](const ExpressionNodePtr& InItem) { return InItem->Expr == VarName; }))
						{
							continue;
						}
						FString TypeName = GetTypeDescStr(VarDesc->TypeDesc);
						const TArray<uint8>& Value = std::get<SpvObject::Internal>(Var->Storage).Value;

						TArray<Vector2i> InitializedRanges = { {0, Value.Num()} };
						if (SDebuggerVariableView::bShowUninitialized)
						{
							InitializedRanges = Var->InitializedRanges;
						}
						FString ValueStr = GetValueStr(Value, VarDesc->TypeDesc, InitializedRanges, 0);
						auto Data = MakeShared<ExpressionNode>(VarName, ValueStr, TypeName);
						TArray<SpvVariableChange::DirtyRange> Ranges;
						DirtyVars.MultiFind(VarId, Ranges);
						if (VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Composite || VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Array
							|| VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
						{
							Data->Children = AppendVarChildNodes(VarDesc->TypeDesc, InitializedRanges, Ranges, Value, 0);
						}

						if (!Ranges.IsEmpty())
						{
							Data->Dirty = true;
						}

						if (!VarDesc->bGlobal)
						{
							LocalVarNodeDatas.Add(MoveTemp(Data));
						}
						else
						{
							GlobalVarNodeDatas.Add(MoveTemp(Data));
						}
					}
				}
			}

		}

		DebuggerLocalVariableView->SetVariableNodeDatas(LocalVarNodeDatas);
		DebuggerGlobalVariableView->SetVariableNodeDatas(GlobalVarNodeDatas);
	}

	bool ShaderDebugger::Continue(StepMode Mode)
	{
		DirtyVars.Empty();
		DebuggerError.Empty();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TSharedPtr<SWindow> ShaderEditorTipWindow = ShEditor->GetShaderEditorTipWindow();
		ShaderEditorTipWindow->HideWindow();

		auto CallStackAtStop = CallStack;
		int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();
		while (CurDebugStateIndex < DebugStates.Num())
		{
			const SpvDebugState& DebugState = DebugStates[CurDebugStateIndex];
			std::optional<int32> NextValidLine;
			if (CurDebugStateIndex + 1 < DebugStates.Num() &&
				((!DebugStates[CurDebugStateIndex + 1].VarChanges.IsEmpty() && !DebugStates[CurDebugStateIndex + 1].bParamChange)
					|| DebugStates[CurDebugStateIndex + 1].bKill
					|| DebugStates[CurDebugStateIndex + 1].bReturn
					|| DebugStates[CurDebugStateIndex + 1].bFuncCallAfterReturn
					|| DebugStates[CurDebugStateIndex + 1].bFuncCall
					|| DebugStates[CurDebugStateIndex + 1].bCondition))
			{
				NextValidLine = DebugStates[CurDebugStateIndex + 1].Line;
			}

			if (!DebugState.UbError.IsEmpty())
			{
				DebuggerError = "Undefined behavior";
				StopLineNumber = DebugState.Line - ExtraLineNum;
				SH_LOG(LogShader, Error, TEXT("%s"), *DebugState.UbError);
				break;
			}

			ApplyDebugState(DebugState);
			CurDebugStateIndex++;

		/*	if (AssertResult)
			{
				TArray<uint8>& Value = std::get<SpvObject::Internal>(AssertResult->Storage).Value;
				uint32& TypedValue = *(uint32*)Value.GetData();
				if (TypedValue != 1)
				{
					DebuggerError = "Assert failed";
					StopLineNumber = DebugState.Line - ExtraLineNum;
					TypedValue = 1;
					break;
				}
			}*/

			if (NextValidLine)
			{
				bool MatchBreakPoint = Scope && ShaderEditor->BreakPointLineNumbers.ContainsByPredicate([&](int32 InEntry) {
					int32 BreakPointLine = InEntry + ExtraLineNum;
					bool bForward;
					if (!CurValidLine)
					{
						int32 FuncLine = GetFunctionDesc(Scope)->GetLine();
						bForward = (BreakPointLine >= FuncLine) && (BreakPointLine <= NextValidLine.value());
					}
					else
					{
						bForward = (BreakPointLine > CurValidLine.value()) && (BreakPointLine <= NextValidLine.value());
					}
					return bForward;
				});
				bool SameStack = CurValidLine != NextValidLine && CallStackAtStop.Num() == CallStack.Num();
				bool ReturnStack = CallStackAtStop.Num() > CallStack.Num();
				bool CrossStack = CallStackAtStop.Num() != CallStack.Num();

				bool StopStepOver = Mode == StepMode::StepOver && NextValidLine.value() - ExtraLineNum > 0 && (SameStack || ReturnStack);
				bool StopStepInto = Mode == StepMode::StepInto && NextValidLine.value() - ExtraLineNum > 0 && (SameStack || CrossStack);

				CurValidLine = NextValidLine;
				if (MatchBreakPoint || StopStepOver || StopStepInto)
				{
					CallStackAtStop = CallStack;
					StopLineNumber = NextValidLine.value() - ExtraLineNum;
					break;
				}
			}
		}

		return CurDebugStateIndex < DebugStates.Num();
	}

	ShaderDebugger::ShaderDebugger(SShaderEditorBox* InShaderEditor)
	: ShaderEditor(InShaderEditor)
	{
	}

	void ShaderDebugger::ApplyDebugState(const SpvDebugState& State)
	{
		CurReturnObject = State.ReturnObject;
		if (State.bFuncCall)
		{
			if (Scope)
			{
				CallStack.Add(TPair<SpvLexicalScope*, int>(Scope, State.Line));
				CurValidLine.reset();
			}
		}

		if (State.ScopeChange)
		{
			Scope = State.ScopeChange.value().NewScope;
			CallStackScope = Scope;
			//Return func
			if (!CallStack.IsEmpty() && State.bReturn)
			{
				auto Item = CallStack.Pop();
				CurValidLine = Item.Value;
			}
		}

		for (const SpvVariableChange& VarChange : State.VarChanges)
		{
			SpvVariable* Var = DebuggerContext->FindVar(VarChange.VarId);
			if (!Var->IsExternal())
			{
				Var->InitializedRanges.Add({ VarChange.Range.OffsetBytes, VarChange.Range.OffsetBytes + VarChange.Range.ByteSize });
				std::get<SpvObject::Internal>(Var->Storage).Value = VarChange.NewValue;
			}
			DirtyVars.Add(VarChange.VarId, VarChange.Range);
		}
	}

	void ShaderDebugger::DebugPixel(const PixelState& InState)
	{
		TRefCountPtr<GpuShader> Shader = ShaderEditor->CreateGpuShader();
		Shader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");

		FString ErrorInfo, WarnInfo;
		if (!GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString() + ":\n\n" + ErrorInfo;
			throw std::runtime_error(TCHAR_TO_ANSI(*FailureInfo));
		}

		ShaderTU TU{ Shader->GetSourceText(), Shader->GetIncludeDirs() };
		Funcs = TU.GetFuncs();

		TArray<SpvBinding> Bindings;
		BindingContext PatchedBindings;
		uint32 BufferSize = 1024 * 1024 * 10;
		TArray<uint8> Datas;
		Datas.SetNumZeroed(BufferSize);
		TRefCountPtr<GpuBuffer> DebuggerBuffer = GGpuRhi->CreateBuffer({
			.ByteSize = BufferSize,
			.Usage = GpuBufferUsage::RWRaw,
			.InitialData = Datas,
		});
		auto SetupBinding = [&](const std::optional<BindingBuilder>& Builder)
		{
			if (!Builder.has_value())
				return;

			GpuBindGroupDesc BindGroupDesc = (*Builder).BingGroupBuilder.GetDesc();
			GpuBindGroupLayoutDesc LayoutDesc = (*Builder).LayoutBuilder.GetDesc();
			int32 SetNumber = BindGroupDesc.Layout->GetGroupNumber();
			for (const auto& [Slot, ResourceBindingEntry] : BindGroupDesc.Resources)
			{
				const auto& LayoutBindingEntry = LayoutDesc.Layouts[Slot];
				Bindings.Add({
					.DescriptorSet = SetNumber,
					.Binding = Slot,
					.Resource = ResourceBindingEntry.Resource
				});
	
				//Replace StructuredBuffer with ByteAddressBuffer
				if (LayoutBindingEntry.Type == BindingType::RWStructuredBuffer)
				{
					uint32 BufferSize = static_cast<GpuBuffer*>(ResourceBindingEntry.Resource)->GetByteSize();
					TArray<uint8> Datas;
					Datas.SetNumZeroed(BufferSize);
					TRefCountPtr<GpuBuffer> RawBuffer = GGpuRhi->CreateBuffer({
						.ByteSize = BufferSize,
						.Usage = GpuBufferUsage::RWRaw,
						.InitialData = Datas,
					});

					BindGroupDesc.Resources[Slot] = { RawBuffer };
					LayoutDesc.Layouts[Slot] = { BindingType::RWRawBuffer, LayoutBindingEntry.Stage };
				}
			}

			if (SetNumber == BindingContext::GlobalSlot)
			{
				//Add the debugger buffer
				BindGroupDesc.Resources.Add(0721, { DebuggerBuffer });
				LayoutDesc.Layouts.Add(0721, { BindingType::RWRawBuffer });

				TRefCountPtr<GpuBindGroupLayout> PatchedBindGroupLayout = GGpuRhi->CreateBindGroupLayout(LayoutDesc);
				BindGroupDesc.Layout = PatchedBindGroupLayout;
				TRefCountPtr<GpuBindGroup> PacthedBindGroup = GGpuRhi->CreateBindGroup(BindGroupDesc);
				PatchedBindings.SetGlobalBindGroup(PacthedBindGroup);
				PatchedBindings.SetGlobalBindGroupLayout(PatchedBindGroupLayout);
			}
			else if (SetNumber == BindingContext::PassSlot)
			{
				TRefCountPtr<GpuBindGroupLayout> PatchedBindGroupLayout = GGpuRhi->CreateBindGroupLayout(LayoutDesc);
				BindGroupDesc.Layout = PatchedBindGroupLayout;
				TRefCountPtr<GpuBindGroup> PacthedBindGroup = GGpuRhi->CreateBindGroup(BindGroupDesc);
				PatchedBindings.SetPassBindGroup(PacthedBindGroup);
				PatchedBindings.SetPassBindGroupLayout(PatchedBindGroupLayout);
			}
			else if (SetNumber == BindingContext::ShaderSlot)
			{
				TRefCountPtr<GpuBindGroupLayout> PatchedBindGroupLayout = GGpuRhi->CreateBindGroupLayout(LayoutDesc);
				BindGroupDesc.Layout = PatchedBindGroupLayout;
				TRefCountPtr<GpuBindGroup> PacthedBindGroup = GGpuRhi->CreateBindGroup(BindGroupDesc);
				PatchedBindings.SetShaderBindGroup(PacthedBindGroup);
				PatchedBindings.SetShaderBindGroupLayout(PatchedBindGroupLayout);
			}
		};
		SetupBinding(InState.GlobalBuilder);
		SetupBinding(InState.PassBuilder);
		SetupBinding(InState.ShaderBuilder);

		PixelDebuggerContext = SpvPixelDebuggerContext{ InState.Coord, Bindings };

		SpirvParser Parser;
		Parser.Parse(Shader->SpvCode);
		SpvMetaVisitor MetaVisitor{ PixelDebuggerContext.value() };
		Parser.Accept(&MetaVisitor);
		SpvPixelDebuggerVisitor PixelDebuggerVisitor{ PixelDebuggerContext.value() };
		Parser.Accept(&PixelDebuggerVisitor);

		const TArray<uint32>& PatchedSpv = PixelDebuggerVisitor.GetPatcher().GetSpv();
		auto EntryPoint = StringCast<UTF8CHAR>(*Shader->GetEntryPoint());
		ShaderConductor::Compiler::TargetDesc HlslTargetDesc{};
		HlslTargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
		HlslTargetDesc.version = "60";
		ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile({ PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 }, (char*)EntryPoint.Get(),
			ShaderConductor::ShaderStage::PixelShader, HlslTargetDesc);
		FString PatchedHlsl = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };
		if (ShaderResultDesc.hasError)
		{
			FString ErrorInfo = static_cast<const char*>(ShaderResultDesc.errorWarningMsg.Data());
			PatchedHlsl = MoveTemp(ErrorInfo);
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString() + ":\n\n" + ErrorInfo;
			throw std::runtime_error(TCHAR_TO_ANSI(*FailureInfo));
		}
#if !SH_SHIPPING
		PixelDebuggerVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / Shader->GetShaderName() / Shader->GetShaderName() + "Patched.spvasm");
		FFileHelper::SaveStringToFile(PatchedHlsl, *(PathHelper::SavedShaderDir() / Shader->GetShaderName() / Shader->GetShaderName() + "Patched.hlsl"));
#endif

		CurDebugStateIndex = 0;
		DebuggerContext = &PixelDebuggerContext.value();
		TRefCountPtr<GpuShader> PatchedShader = GGpuRhi->CreateShaderFromSource({
			.Name = "PacthedShader",
			.Source = MoveTemp(PatchedHlsl),
			.Type = Shader->GetShaderType(),
			.EntryPoint = "main"
		});
		if (!GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo))
		{
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString() + ":\n\n" + ErrorInfo;
			throw std::runtime_error(TCHAR_TO_ANSI(*FailureInfo));
		}

		GpuRenderPipelineStateDesc PatchedPipelineDesc{
			.Vs = InState.PipelineDesc.Vs,
			.Ps = PatchedShader,
			.RasterizerState = InState.PipelineDesc.RasterizerState,
			.Primitive = InState.PipelineDesc.Primitive
		};
		PatchedBindings.ApplyBindGroupLayout(PatchedPipelineDesc);
		TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PatchedPipelineDesc);
		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass({}, TEXT("Debugger"));
			{
				PassRecorder->SetViewPort(InState.ViewPortDesc);
				PassRecorder->SetRenderPipelineState(Pipeline);
				PatchedBindings.ApplyBindGroup(PassRecorder);
				InState.DrawFunction(PassRecorder);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });

		uint8* DebuggerBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebuggerBuffer, GpuResourceMapMode::Read_Only);
		GenDebugStates(DebuggerBufferData);
		GGpuRhi->UnMapGpuBuffer(DebuggerBuffer);

		InitDebuggerView();

		for (const auto& [VarId, VarDesc] : DebuggerContext->VariableDescMap)
		{
			if (VarDesc)
			{
				SortedVariableDescs.emplace_back(VarId, VarDesc);
				if (VarDesc->Name == "GPrivate_AssertResult")
				{
					AssertResult = DebuggerContext->FindVar(VarId);;
				}
			}
		}
		std::sort(SortedVariableDescs.begin(), SortedVariableDescs.end(), [](const auto& PairA, const auto& PairB) {
			return PairA.second->Line > PairB.second->Line;
		});
	}

	void ShaderDebugger::DebugCompute(const ComputeState& InState)
	{

	}

	void ShaderDebugger::Reset()
	{
		CurValidLine.reset();
		CallStack.Empty();
		DebuggerContext = nullptr;
		Scope = nullptr;
		CallStackScope = nullptr;
		CurReturnObject.reset();
		AssertResult = nullptr;
		DebuggerError.Empty();
		DirtyVars.Empty();
		StopLineNumber = 0;
		PixelDebuggerContext.reset();
		DebugStates.Reset();
		SortedVariableDescs.clear();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		DebuggerLocalVariableView->SetVariableNodeDatas({});
		DebuggerGlobalVariableView->SetVariableNodeDatas({});
		DebuggerLocalVariableView->SetOnShowUninitialized(nullptr);
		DebuggerGlobalVariableView->SetOnShowUninitialized(nullptr);
	}

	void ShaderDebugger::InitDebuggerView()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerCallStackView* DebuggerCallStackView = ShEditor->GetDebuggerCallStackView();
		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();

		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		DebuggerLocalVariableView->SetOnShowUninitialized([this](bool bShowUninitialized) { ShowDeuggerVariable(CallStackScope); });
		DebuggerGlobalVariableView->SetOnShowUninitialized([this](bool bShowUninitialized) { ShowDeuggerVariable(CallStackScope); });

		DebuggerCallStackView->OnSelectionChanged = [this, DebuggerWatchView](const FString& FuncName) {
			int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();
			if (GetFunctionSig(GetFunctionDesc(Scope), Funcs) == FuncName)
			{
				StopLineNumber = CurValidLine.value() - ExtraLineNum;
				ShowDeuggerVariable(Scope);
				CallStackScope = Scope;
			}
			else if (auto Call = CallStack.FindByPredicate([this, FuncName](const TPair<FW::SpvLexicalScope*, int>& InItem) {
				if (GetFunctionSig(GetFunctionDesc(InItem.Key), Funcs) == FuncName)
				{
					return true;
				}
				return false;
			}))
			{
				StopLineNumber = Call->Value - ExtraLineNum;
				ShowDeuggerVariable(Call->Key);
				CallStackScope = Call->Key;
			}
			DebuggerWatchView->Refresh();
		};
	}

	void ShaderDebugger::GenDebugStates(uint8* DebuggerData)
	{
		int Offset = 0;
		SpvDebuggerStateType StateType = *(SpvDebuggerStateType*)(DebuggerData + Offset);
		SpvLexicalScope* ScopeState = nullptr;
		while (StateType != SpvDebuggerStateType::None)
		{
			Offset += sizeof(SpvDebuggerStateType);
			switch (StateType)
			{
			case SpvDebuggerStateType::ScopeChange:
			{
				SpvScopeChangeState State = *(SpvScopeChangeState*)(DebuggerData + Offset);
				SpvLexicalScope* NewScopeState = PixelDebuggerContext.value().LexicalScopes[State.ScopeId].Get();
				DebugStates.Add({
					.Line = State.Line,
					.ScopeChange = SpvLexicalScopeChange {
						.PreScope = ScopeState,
						.NewScope = NewScopeState,
					}
					});
				ScopeState = NewScopeState;
				Offset += sizeof(SpvScopeChangeState);
			}
			break;
			case SpvDebuggerStateType::Condition:
			{
				SpvOtherState State = *(SpvOtherState*)(DebuggerData + Offset);
				DebugStates.Add({
					.Line = State.Line,
					.bCondition = true,
					});
				Offset += sizeof(SpvOtherState);
			}
			break;
			default:
				AUX::Unreachable();
			}
			StateType = *(SpvDebuggerStateType*)(DebuggerData + Offset);
		}
	}

}
