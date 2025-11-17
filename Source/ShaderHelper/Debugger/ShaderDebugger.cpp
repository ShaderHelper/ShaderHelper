#include "CommonHeader.h"
#include "ShaderDebugger.h"
#include "ShaderConductor.hpp"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "GpuApi/Spirv/SpirvExprDebugger.h"

using namespace FW;

namespace SH
{
	TArray<ExpressionNodePtr> AppendVarChildNodes(SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, const TArray<SpvVarDirtyRange>& DirtyRanges, const TArray<uint8>& Value, int32 InOffset)
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
				int32 RangeStart = Range.ByteOffset;
				int32 RangeEnd = Range.ByteOffset + Range.ByteSize;
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
					int32 RangeStart = Range.ByteOffset;
					int32 RangeEnd = Range.ByteOffset + Range.ByteSize;
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
					int32 RangeStart = Range.ByteOffset;
					int32 RangeEnd = Range.ByteOffset + Range.ByteSize;
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
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		ExtraArgs.Add("/Od");
		FString ErrorInfo, WarnInfo;

		if (DebugShader->GetShaderType() == ShaderType::PixelShader)
		{
			SpvPixelExprDebuggerContext ExprContext{ DebugStates[CurDebugStateIndex], CurDebugStateIndex,
				PsState.value().Coord, Funcs, SpvBindings };
			SpirvParser Parser;
			Parser.Parse(DebugShader->SpvCode);
			SpvMetaVisitor MetaVisitor{ ExprContext };
			Parser.Accept(&MetaVisitor);
			SpvPixelExprDebuggerVisitor ExprVisitor{ ExprContext };
			Parser.Accept(&ExprVisitor);
			ExprVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "ExprPatched.spvasm");
			
			const TArray<uint32>& PatchedSpv = ExprVisitor.GetPatcher().GetSpv();
			auto EntryPoint = StringCast<UTF8CHAR>(*DebugShader->GetEntryPoint());
			ShaderConductor::Compiler::TargetDesc HlslTargetDesc{};
			HlslTargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
			HlslTargetDesc.version = "60";
			ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile({.force_zero_initialized_variables = true}, { PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 }, (char*)EntryPoint.Get(),
				ShaderConductor::ShaderStage::PixelShader, HlslTargetDesc);
			FString PatchedHlsl = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };

			if (!ShaderResultDesc.hasError)
			{
				FString Pattern = "_AppendExprDummy_()";
				int32 ReplaceIndex = PatchedHlsl.Find(Pattern, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				PatchedHlsl.RemoveAt(ReplaceIndex, Pattern.Len());
				PatchedHlsl.InsertAt(ReplaceIndex, "_AppendExpr_(" + InExpression + ")");
				int32 RemoveStartIndex = PatchedHlsl.Find("void _AppendExprDummy_()", ESearchCase::IgnoreCase, ESearchDir::FromEnd, ReplaceIndex);
				int32 RemoveEndIndex = PatchedHlsl.Find("}", ESearchCase::IgnoreCase, ESearchDir::FromStart, RemoveStartIndex);
				PatchedHlsl.RemoveAt(RemoveStartIndex, RemoveEndIndex - RemoveStartIndex + 1);
				FFileHelper::SaveStringToFile(PatchedHlsl, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "ExprPatched.hlsl"));

				TRefCountPtr<GpuShader> PatchedShader = GGpuRhi->CreateShaderFromSource({
					.Source = MoveTemp(PatchedHlsl),
					.Type = DebugShader->GetShaderType(),
					.EntryPoint = DebugShader->GetEntryPoint()
				});
				if (GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo, ExtraArgs))
				{
					GpuRenderPipelineStateDesc PatchedPipelineDesc{
						.Vs = PsState.value().PipelineDesc.Vs,
						.Ps = PatchedShader,
						.RasterizerState = PsState.value().PipelineDesc.RasterizerState,
						.Primitive = PsState.value().PipelineDesc.Primitive
					};
					PatchedBindings.ApplyBindGroupLayout(PatchedPipelineDesc);
					TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PatchedPipelineDesc);

					auto CmdRecorder = GGpuRhi->BeginRecording();
					{
						GpuResourceHelper::ClearRWResource(CmdRecorder, DebugBuffer);
						CmdRecorder->Barriers({ GpuBarrierInfo{.Resource = DebugBuffer, .NewState = GpuResourceState::UnorderedAccess} });
						auto PassRecorder = CmdRecorder->BeginRenderPass({}, TEXT("ExprDebugger"));
						{
							PassRecorder->SetViewPort(PsState.value().ViewPortDesc);
							PassRecorder->SetRenderPipelineState(Pipeline);
							PatchedBindings.ApplyBindGroup(PassRecorder);
							PsState.value().DrawFunction(PassRecorder);
						}
						CmdRecorder->EndRenderPass(PassRecorder);
					}
					GGpuRhi->EndRecording(CmdRecorder);
					GGpuRhi->Submit({ CmdRecorder });

					uint8* DebugBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebugBuffer, GpuResourceMapMode::Read_Only);
					SpvId TypeDescId = *(uint32*)(DebugBufferData);
					int32 ResultSize = *(int32*)(DebugBufferData + 4);
					TArray<uint8> ResultValue = { DebugBufferData + 8, ResultSize };
					GGpuRhi->UnMapGpuBuffer(DebugBuffer);

					SpvTypeDesc* ResultTypeDesc = ExprContext.TypeDescs[TypeDescId].Get();
					if (ResultTypeDesc)
					{
						FText TypeName = LOCALIZATION("InvalidExpr");
						TypeName = FText::FromString(GetTypeDescStr(ResultTypeDesc));

						TArray<Vector2i> ResultRange;
						ResultRange.Add({ 0, ResultValue.Num() });
						FString ValueStr = GetValueStr(ResultValue, ResultTypeDesc, ResultRange, 0);

						TArray<TSharedPtr<ExpressionNode>> Children;
						if (ResultTypeDesc->GetKind() == SpvTypeDescKind::Composite || ResultTypeDesc->GetKind() == SpvTypeDescKind::Array
							|| ResultTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
						{
							Children = AppendExprChildNodes(ResultTypeDesc, ResultRange, ResultValue, 0);
						}

						return { .Expr = InExpression, .ValueStr = ValueStr,
							.TypeName = TypeName.ToString(), .Children = MoveTemp(Children) };
					}
		
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

		ShowDeuggerVariable(Scope);

		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();
		DebuggerWatchView->OnWatch = [this](const FString& Expression) { return EvaluateExpression(Expression); };
		DebuggerWatchView->Refresh();
	}

	void ShaderDebugger::ShowDeuggerVariable(SpvLexicalScope* InScope) const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();

		TArray<ExpressionNodePtr> LocalVarNodeDatas;
		TArray<ExpressionNodePtr> GlobalVarNodeDatas;

		if (!ReturnValue.IsEmpty())
		{
			SpvTypeDesc* ReturnTypeDesc = std::get<SpvTypeDesc*>(GetFunctionDesc(Scope)->GetFuncTypeDesc()->GetReturnType());
			FString VarName = GetFunctionDesc(Scope)->GetName() + LOCALIZATION("ReturnTip").ToString();
			FString TypeName = GetTypeDescStr(ReturnTypeDesc);
			FString ValueStr = GetValueStr(ReturnValue, ReturnTypeDesc, TArray{ Vector2i{0, ReturnValue.Num()} }, 0);

			auto Data = MakeShared<ExpressionNode>(VarName, ValueStr, TypeName);
			LocalVarNodeDatas.Add(MoveTemp(Data));
		}

		for (const auto& [VarId, VarDesc] : SortedVariableDescs)
		{
			if (SpvVariable* Var = DebuggerContext->FindVar(VarId))
			{
				if (!Var->IsExternal())
				{
					bool VisibleScope = VarDesc->Parent->Contains(InScope);
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
						TArray<SpvVarDirtyRange> Ranges;
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

		TArray<int32> ValidLines;
		for (const auto& [_, BB] : DebuggerContext->BBs)
		{
			ValidLines.Append(BB.ValidLines);
		}
		ValidLines.Sort();
		auto CallStackAtStop = CallStack;
		int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();
		while (CurDebugStateIndex < DebugStates.Num())
		{
			const SpvDebugState& DebugState = DebugStates[CurDebugStateIndex];
			std::optional<int32> NextValidLine;
			if (CurDebugStateIndex + 1 < DebugStates.Num())
			{
				const SpvDebugState& NextDebugState = DebugStates[CurDebugStateIndex + 1];
				if (std::holds_alternative<SpvDebugState_VarChange>(NextDebugState))
				{
					NextValidLine = std::get<SpvDebugState_VarChange>(NextDebugState).Line;
				}
				else if (std::holds_alternative<SpvDebugState_Tag>(NextDebugState))
				{
					NextValidLine = std::get<SpvDebugState_Tag>(NextDebugState).Line;
				}
			}

			/*if (!DebugState.UbError.IsEmpty())
			{
				DebuggerError = "Undefined behavior";
				StopLineNumber = DebugState.Line - ExtraLineNum;
				SH_LOG(LogShader, Error, TEXT("%s"), *DebugState.UbError);
				break;
			}*/

			ApplyDebugState(DebugState);
			CurDebugStateIndex++;

			if (AssertResult && AssertResult->IsCompletelyInitialized())
			{
				TArray<uint8>& Value = AssertResult->GetBuffer();
				uint32& TypedValue = *(uint32*)Value.GetData();
				if (TypedValue != 1)
				{
					DebuggerError = "Assert failed";
					StopLineNumber = CurValidLine.value() - ExtraLineNum;
					TypedValue = 1;
					break;
				}
			}

			if (NextValidLine && NextValidLine.value())
			{
				bool MatchBreakPoint = Scope && ShaderEditor->BreakPointLineNumbers.ContainsByPredicate([&](int32 InEntry) {
					int32 BreakPointLine = InEntry + ExtraLineNum;
					int32 BreakPointValidLine = ValidLines[Algo::UpperBound(ValidLines, BreakPointLine - 1)];
					for (const auto& [_, BB] : DebuggerContext->BBs)
					{
						if (BB.ValidLines.Contains(BreakPointValidLine) && BB.ValidLines.Contains(NextValidLine))
						{
							if (!CurValidLine)
							{
								int32 FuncLine = GetFunctionDesc(Scope)->GetLine();
								return (BreakPointValidLine >= FuncLine) && (BreakPointValidLine <= NextValidLine.value());
							}
							else
							{
								return (BreakPointValidLine > CurValidLine.value()) && (BreakPointValidLine <= NextValidLine.value());
							}
						}
					}
					return false;
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

	bool ShaderDebugger::EnableUbsan()
	{
		bool EnableUbsan = false;
		Editor::GetEditorConfig()->GetBool(TEXT("Debugger"), TEXT("EnableUbsan"), EnableUbsan);
		return EnableUbsan;
	}

	void ShaderDebugger::ApplyDebugState(const SpvDebugState& InState)
	{
		if (std::holds_alternative<SpvDebugState_VarChange>(InState))
		{
			const auto& State = std::get<SpvDebugState_VarChange>(InState);
			SpvVariable* Var = DebuggerContext->FindVar(State.Change.VarId);

			void* Dest = &Var->GetBuffer()[State.Change.ByteOffset];
			const void* Src = State.Change.NewDirtyValue.GetData();
			FMemory::Memcpy(Dest, Src, State.Change.NewDirtyValue.Num());

			SpvVarDirtyRange DirtyRange = { State.Change.ByteOffset, State.Change.NewDirtyValue.Num() };
			Var->InitializedRanges.Add({ State.Change.ByteOffset, State.Change.NewDirtyValue.Num() });
			DirtyVars.Add(State.Change.VarId, MoveTemp(DirtyRange));
		}
		else if (std::holds_alternative<SpvDebugState_ReturnValue>(InState))
		{
			const auto& State = std::get<SpvDebugState_ReturnValue>(InState);
			ReturnValue = State.Value;
		}
		else if (std::holds_alternative<SpvDebugState_ScopeChange>(InState))
		{
			const auto& State = std::get<SpvDebugState_ScopeChange>(InState);
			Scope = State.Change.NewScope;
			CallStackScope = Scope;
		}
		else if (std::holds_alternative<SpvDebugState_Tag>(InState))
		{
			const auto& State = std::get<SpvDebugState_Tag>(InState);
			if (State.bFuncCall && Scope)
			{
				CallStack.Add(TPair<SpvLexicalScope*, int>(Scope, State.Line));
				CurValidLine.reset();
			}
			else if (State.bReturn && !CallStack.IsEmpty())
			{
				auto Item = CallStack.Pop();
				CurValidLine = Item.Value;
				ReturnValue.Empty();
			}
		}
	}

	void ShaderDebugger::DebugPixel(const BindingState& InBuilders, const PixelState& InState)
	{
		BindingBuilders = InBuilders;
		PsState = InState;
		DebugShader = ShaderEditor->CreateGpuShader();
		DebugShader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		ExtraArgs.Add("/Od");

		FString ErrorInfo, WarnInfo;
		if (!GGpuRhi->CompileShader(DebugShader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString();
			SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo, *ErrorInfo);
			throw std::runtime_error(TCHAR_TO_UTF8(*FailureInfo));
		}

		ShaderTU TU{ DebugShader->GetSourceText(), DebugShader->GetIncludeDirs() };
		Funcs = TU.GetFuncs();

		uint32 BufferSize = 1024 * 1024 * 2;
		TArray<uint8> Datas;
		Datas.SetNumZeroed(BufferSize);
		DebugBuffer = GGpuRhi->CreateBuffer({
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
				SpvBindings.Add({
					.DescriptorSet = SetNumber,
					.Binding = Slot,
					.Resource = ResourceBindingEntry.Resource
				});
	
				//Replace StructuredBuffer with ByteAddressBuffer
				if (LayoutBindingEntry.Type == BindingType::RWStructuredBuffer)
				{
					uint32 BufferSize = static_cast<GpuBuffer*>(ResourceBindingEntry.Resource.GetReference())->GetByteSize();
					TArray<uint8> Datas;
					Datas.SetNumZeroed(BufferSize);
					TRefCountPtr<GpuBuffer> RawBuffer = GGpuRhi->CreateBuffer({
						.ByteSize = BufferSize,
						.Usage = GpuBufferUsage::RWRaw,
						.InitialData = Datas,
					});

					BindGroupDesc.Resources[Slot] = { AUX::StaticCastRefCountPtr<GpuResource>(RawBuffer) };
					LayoutDesc.Layouts[Slot] = { BindingType::RWRawBuffer, LayoutBindingEntry.Stage };
				}
			}

			if (SetNumber == BindingContext::GlobalSlot)
			{
				//Add the debugger buffer
				BindGroupDesc.Resources.Add(0721, { AUX::StaticCastRefCountPtr<GpuResource>(DebugBuffer) });
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
		SetupBinding(InBuilders.GlobalBuilder);
		SetupBinding(InBuilders.PassBuilder);
		SetupBinding(InBuilders.ShaderBuilder);

		PixelDebuggerContext = SpvPixelDebuggerContext{InState.Coord, Funcs, SpvBindings };

		SpirvParser Parser;
		Parser.Parse(DebugShader->SpvCode);
		SpvMetaVisitor MetaVisitor{ PixelDebuggerContext.value() };
		Parser.Accept(&MetaVisitor);
		SpvPixelDebuggerVisitor PixelDebuggerVisitor{ PixelDebuggerContext.value(), EnableUbsan()};
		Parser.Accept(&PixelDebuggerVisitor);
		PixelDebuggerVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Patched.spvasm");

		const TArray<uint32>& PatchedSpv = PixelDebuggerVisitor.GetPatcher().GetSpv();
		auto EntryPoint = StringCast<UTF8CHAR>(*DebugShader->GetEntryPoint());
		ShaderConductor::Compiler::TargetDesc HlslTargetDesc{};
		HlslTargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
		HlslTargetDesc.version = "60";
		ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile({}, { PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 }, (char*)EntryPoint.Get(),
			ShaderConductor::ShaderStage::PixelShader, HlslTargetDesc);
		FString PatchedHlsl = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };
		FFileHelper::SaveStringToFile(PatchedHlsl, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Patched.hlsl"));

		if (ShaderResultDesc.hasError)
		{
			FString ErrorInfo = static_cast<const char*>(ShaderResultDesc.errorWarningMsg.Data());
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString();
			SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo, *ErrorInfo);
			throw std::runtime_error(TCHAR_TO_UTF8(*FailureInfo));
		} 

		CurDebugStateIndex = 0;
		DebuggerContext = &PixelDebuggerContext.value();
		TRefCountPtr<GpuShader> PatchedShader = GGpuRhi->CreateShaderFromSource({
			.Source = MoveTemp(PatchedHlsl),
			.Type = DebugShader->GetShaderType(),
			.EntryPoint = DebugShader->GetEntryPoint()
		});
		if (!GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString();
			SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo, *ErrorInfo);
			throw std::runtime_error(TCHAR_TO_UTF8(*FailureInfo));
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

		uint8* DebugBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebugBuffer, GpuResourceMapMode::Read_Only);
		GenDebugStates(DebugBufferData);
		GGpuRhi->UnMapGpuBuffer(DebugBuffer);

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

	void ShaderDebugger::DebugCompute(const BindingState& InBuilders, const ComputeState& InState)
	{

	}

	void ShaderDebugger::Reset()
	{
		CurValidLine.reset();
		CallStack.Empty();
		DebuggerContext = nullptr;
		Scope = nullptr;
		CallStackScope = nullptr;
		AssertResult = nullptr;
		DebuggerError.Empty();
		DirtyVars.Empty();
		StopLineNumber = 0;
		ReturnValue.Empty();
		PixelDebuggerContext.reset();
		DebugStates.Reset();
		SortedVariableDescs.clear();
		PsState.reset();
		SpvBindings.Empty();
		BindingBuilders = {};
		PatchedBindings = {};
		DebugShader = nullptr;
		DebugBuffer = nullptr;

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
		SpvLexicalScope* PreScope = nullptr;
		while (StateType != SpvDebuggerStateType::None)
		{
			Offset += sizeof(SpvDebuggerStateType);
			switch (StateType)
			{
			case SpvDebuggerStateType::VarChange:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				SpvId VarId = *(SpvId*)(DebuggerData + Offset);
				Offset += 4;
				int32 IndexNum = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				TArray<uint32> Indexes = { (uint32*)(DebuggerData + Offset), IndexNum};
				Offset += IndexNum * 4;
				int32 ValueSize = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				TArray<uint8> DirtyValue = {DebuggerData + Offset, ValueSize};
				Offset += DirtyValue.Num();

				SpvVariable* DirtyVar = DebuggerContext->FindVar(VarId);
				int32 ByteOffset = GetByteOffset(DirtyVar, Indexes);
				TArray<uint8> PreDirtyValue = { &DirtyVar->GetBuffer()[ByteOffset], ValueSize};

				auto NewDebugState = SpvDebugState_VarChange{
					.Line = Line,
					.Change = {
						.VarId = VarId,
						.PreDirtyValue = MoveTemp(PreDirtyValue),
						.NewDirtyValue = MoveTemp(DirtyValue),
						.ByteOffset = ByteOffset,
					},
				};
				DebugStates.Add(MoveTemp(NewDebugState));
				break;
			}
			case SpvDebuggerStateType::ScopeChange:
			{
				SpvId ScopeId = *(SpvId*)(DebuggerData + Offset);
				Offset += 4;
				SpvLexicalScope* NewScope = PixelDebuggerContext.value().LexicalScopes[ScopeId].Get();
				DebugStates.Add(SpvDebugState_ScopeChange{
					{
						.PreScope = PreScope,
						.NewScope = NewScope,
					}
				});
				PreScope = NewScope;
				break;
			}
			case SpvDebuggerStateType::ReturnValue:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				int32 ValueSize = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				TArray<uint8> ReturnValue = { DebuggerData + Offset, ValueSize };
				Offset += ReturnValue.Num();
				DebugStates.Add(SpvDebugState_ReturnValue{
					.Line = Line,
					.Value = MoveTemp(ReturnValue)
				});
				break;
			}
			case SpvDebuggerStateType::Condition:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				DebugStates.Add(SpvDebugState_Tag{
					.Line = Line,
					.bCondition = true,
				});
				break;
			}
			case SpvDebuggerStateType::FuncCall:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				DebugStates.Add(SpvDebugState_Tag{
					.Line = Line,
					.bFuncCall = true,
				});
				break;
			}
			case SpvDebuggerStateType::FuncCallAfterReturn:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				DebugStates.Add(SpvDebugState_Tag{
					.Line = Line,
					.bFuncCallAfterReturn = true,
				});
				break;
			}
			case SpvDebuggerStateType::Return:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				DebugStates.Add(SpvDebugState_Tag{
					.Line = Line,
					.bReturn = true,
				});
				break;
			}
			case SpvDebuggerStateType::Kill:
			{
				int32 Line = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				DebugStates.Add(SpvDebugState_Tag{
					.Line = Line,
					.bKill = true,
				});
				break;
			}
			default:
				AUX::Unreachable();
			}
			StateType = *(SpvDebuggerStateType*)(DebuggerData + Offset);
		}
	}

}
