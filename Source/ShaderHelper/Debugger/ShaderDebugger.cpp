#include "CommonHeader.h"
#include "ShaderDebugger.h"
#include "ShaderConductor.hpp"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/Spirv/SpirvExprDebugger.h"
#include "GpuApi/Spirv/SpirvValidator.h"

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
		ExtraArgs.Add("-ignore-validation-error");
		FString ErrorInfo, WarnInfo;

		if (DebugShader->GetShaderType() == ShaderType::PixelShader)
		{
			const auto& PsInvocation = std::get<PixelState>(Invocation);
			SpvPixelExprDebuggerContext ExprContext{ DebugStates[CurDebugStateIndex], CurDebugStateIndex,
				PixelCoord, SpvBindings };
			SpirvParser Parser;
			Parser.Parse(DebugShader->SpvCode);
			SpvMetaVisitor MetaVisitor{ ExprContext };
			Parser.Accept(&MetaVisitor);
			SpvPixelExprDebuggerVisitor ExprVisitor{ ExprContext, bEnableUbsan };
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
						.Vs = PsInvocation.PipelineDesc.Vs,
						.Ps = PatchedShader,
						.RasterizerState = PsInvocation.PipelineDesc.RasterizerState,
						.Primitive = PsInvocation.PipelineDesc.Primitive
					};
					PatchedBindings.ApplyBindGroupLayout(PatchedPipelineDesc);
					TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PatchedPipelineDesc);

					auto CmdRecorder = GGpuRhi->BeginRecording();
					{
						GpuResourceHelper::ClearRWResource(CmdRecorder, DebugBuffer);
						CmdRecorder->Barriers({ GpuBarrierInfo{.Resource = DebugBuffer, .NewState = GpuResourceState::UnorderedAccess} });
						auto PassRecorder = CmdRecorder->BeginRenderPass({}, TEXT("ExprDebugger"));
						{
							PassRecorder->SetViewPort(PsInvocation.ViewPortDesc);
							PassRecorder->SetRenderPipelineState(Pipeline);
							PatchedBindings.ApplyBindGroup(PassRecorder);
							PsInvocation.DrawFunction(PassRecorder);
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
					if (ResultTypeDesc && !ResultValue.IsEmpty())
					{
						 FText TypeName = FText::FromString(GetTypeDescStr(ResultTypeDesc));

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
		CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc), MoveTemp(Location)));
		for (int i = CallStack.Num() - 1; i >= 0; i--)
		{
			SpvFunctionDesc* FuncDesc = GetFunctionDesc(CallStack[i].Scope);
			int JumpLineNumber = CallStack[i].Line - ExtraLineNum;
			if (JumpLineNumber > 0)
			{
				FString Location = FString::Printf(TEXT("%s (Line %d)"), *ShaderAssetObj->GetFileName(), JumpLineNumber);
				CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc), MoveTemp(Location)));
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
						if (Value.IsEmpty())
						{
							continue;
						}

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
		//Allow ignoring assert failures and proceeding.
		if (DebuggerError == "Assert failed")
		{
			CurDebugStateIndex++;
		}
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
				else if (std::holds_alternative<SpvDebugState_FuncCall>(NextDebugState))
				{
					NextValidLine = std::get<SpvDebugState_FuncCall>(NextDebugState).Line;
				}
				else if (std::holds_alternative<SpvDebugState_Tag>(NextDebugState))
				{
					NextValidLine = std::get<SpvDebugState_Tag>(NextDebugState).Line;
				}
			}

			FString Error;
			ApplyDebugState(DebugState, Error);
			if (!Error.IsEmpty())
			{
				DebuggerError = MoveTemp(Error);
				break;
			}
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
			CurDebugStateIndex++;

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
								return (BreakPointLine >= FuncLine) && (BreakPointLine <= NextValidLine.value());
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

		return CurDebugStateIndex < DebugStates.Num() && StopLineNumber > 0;
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

	void ShaderDebugger::ApplyDebugState(const SpvDebugState& InState, FString& Error)
	{
		int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();
		if (std::holds_alternative<SpvDebugState_VarChange>(InState))
		{
			const auto& State = std::get<SpvDebugState_VarChange>(InState);
			if (!State.Error.IsEmpty())
			{
				if (bEnableUbsan)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("%s"), *State.Error);
				}
				return;
			}

			SpvVariable* Var = DebuggerContext->FindVar(State.Change.VarId);

			void* Dest = &Var->GetBuffer()[State.Change.ByteOffset];
			const void* Src = State.Change.NewDirtyValue.GetData();
			FMemory::Memcpy(Dest, Src, State.Change.NewDirtyValue.Num());

			SpvVarDirtyRange DirtyRange = { State.Change.ByteOffset, State.Change.NewDirtyValue.Num() };
			Var->InitializedRanges.Add({ State.Change.ByteOffset, State.Change.NewDirtyValue.Num() });
			DirtyVars.Add(State.Change.VarId, MoveTemp(DirtyRange));

			//Propagate changes made to the parameter back to the argument.
			//This assumes that OpFunctionParameter's result type is always a pointer type
			if (DebuggerContext->IsParameter(Var->Id) && !CallStack.IsEmpty())
			{
				const SpvFuncCall* SourceCall = CallStack.Last().Call;
				const TArray<SpvId>& Parameters = DebuggerContext->Funcs.at(SourceCall->Callee).Parameters;
				for (int i = 0; i < SourceCall->Arguments.Num(); i++)
				{
					if (Parameters[i] == Var->Id)
					{
						SpvVariable* ArgumentVar = DebuggerContext->FindVar(SourceCall->Arguments[i]);
						ArgumentVar->Storage = Var->Storage;
						ArgumentVar->InitializedRanges = Var->InitializedRanges;
						break;
					}
				}
			}
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
			ActiveCallStackScope = Scope;
		}
		else if (std::holds_alternative<SpvDebugState_FuncCall>(InState))
		{
			const auto& State = std::get<SpvDebugState_FuncCall>(InState);
			const SpvFuncCall& Call = DebuggerContext->FuncCalls.at(State.CallId);
			const TArray<SpvId>& Parameters = DebuggerContext->Funcs.at(Call.Callee).Parameters;
			//Assign arguments to parameters.
			for (int i = 0; i < Call.Arguments.Num(); i++)
			{
				SpvVariable* ArgumentVar = DebuggerContext->FindVar(Call.Arguments[i]);
				SpvVariable* ParameterVar = DebuggerContext->FindVar(Parameters[i]);
				ParameterVar->Storage = ArgumentVar->Storage;
				ParameterVar->InitializedRanges = ArgumentVar->InitializedRanges;
			}
			if (Scope)
			{
				CallStack.Emplace(Scope, State.Line, &Call);
				CurValidLine.reset();
			}
		}
		else if (std::holds_alternative<SpvDebugState_Tag>(InState))
		{
			const auto& State = std::get<SpvDebugState_Tag>(InState);
			if (State.bReturn && !CallStack.IsEmpty())
			{
				auto Item = CallStack.Pop();
				CurValidLine = Item.Line;
				ReturnValue.Empty();
			}
		}
		//UBSan
		else if (std::holds_alternative<SpvDebugState_Access>(InState))
		{
			const auto& State = std::get<SpvDebugState_Access>(InState);
			SpvVariable* Var = DebuggerContext->FindVar(State.VarId);
			if (Var->GetBufferSize() > 0)
			{
				auto Range = Var->GetInitializedRange();
				auto AccessOrError = GetAccess(Var, State.Indexes);
				if (AccessOrError.HasError())
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("%s"), *AccessOrError.GetError());
				}
				else
				{
					auto [Type, ByteOffset] = AccessOrError.GetValue();
					int32 Size = GetTypeByteSize(Type);
					if (ByteOffset < Range.X || ByteOffset + Size > Range.Y)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Reading the unintialized memory of the variable: %s!"), *DebuggerContext->Names[Var->Id]);
					}
				}
	
			}

		}
		else if (std::holds_alternative<SpvDebugState_Normalize>(InState))
		{
			const auto& State = std::get<SpvDebugState_Normalize>(InState);
			
			TArray<uint8> ZeroBuffer;
			ZeroBuffer.SetNumZeroed(State.X.Num());
			if (FMemory::Memcmp(State.X.GetData(), ZeroBuffer.GetData(), ZeroBuffer.Num()) == 0)
			{
				Error = "Undefined";
				StopLineNumber = State.Line - ExtraLineNum;
				SH_LOG(LogShader, Error, TEXT("normalize: normalizing a zero scalar or vector is undefined."));
			}
		}
		else if (std::holds_alternative<SpvDebugState_Pow>(InState))
		{
			const auto& State = std::get<SpvDebugState_Pow>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float X = *(float*)(State.X.GetData() + i * 4);
					float Y = *(float*)(State.Y.GetData() + i * 4);
					if (X < 0.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("pow: x(%f) < 0 for component %d, the result is undefined."), X, i);
					}
					else if (X == 0.0f && Y <= 0.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("pow: x = 0 and y(%f) <= 0 for component %d, the result is undefined."), Y, i);
					}
				}
			}
			else
			{
				float X = *(float*)(State.X.GetData());
				float Y = *(float*)(State.Y.GetData());
				if (X < 0.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("pow: x(%f) < 0, the result is undefined."), X);
				}
				else if (X == 0.0f && Y <= 0.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("pow: x = 0 and y(%f) <= 0, the result is undefined."), Y);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Clamp>(InState))
		{
			const auto& State = std::get<SpvDebugState_Clamp>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					if (ResultType->GetKind() == SpvTypeKind::Float)
					{
						float MinVal = *(float*)(State.MinVal.GetData() + i * 4);
						float MaxVal = *(float*)(State.MaxVal.GetData() + i * 4);
						if (MinVal > MaxVal)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("clamp: minVal(%f) > maxVal(%f) for component %d, the result is undefined."), MinVal, MaxVal, i);
						}
					}
					else if (ResultType->GetKind() == SpvTypeKind::Integer)
					{
						if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
						{
							int MinVal = *(int*)(State.MinVal.GetData() + i * 4);
							int MaxVal = *(int*)(State.MaxVal.GetData() + i * 4);
							if (MinVal > MaxVal)
							{
								Error = "Undefined";
								StopLineNumber = State.Line - ExtraLineNum;
								SH_LOG(LogShader, Error, TEXT("clamp: minVal(%d) > maxVal(%d) for component %d, the result is undefined."), MinVal, MaxVal, i);
							}
						}
						else
						{
							uint32 MinVal = *(uint32*)(State.MinVal.GetData() + i * 4);
							uint32 MaxVal = *(uint32*)(State.MaxVal.GetData() + i * 4);
							if (MinVal > MaxVal)
							{
								Error = "Undefined";
								StopLineNumber = State.Line - ExtraLineNum;
								SH_LOG(LogShader, Error, TEXT("clamp: minVal(%d) > maxVal(%d) for component %d, the result is undefined."), MinVal, MaxVal, i);
							}
						}
					}
				}
			}
			else
			{
				if (ResultType->GetKind() == SpvTypeKind::Float)
				{
					float MinVal = *(float*)(State.MinVal.GetData());
					float MaxVal = *(float*)(State.MaxVal.GetData());
					if (MinVal > MaxVal)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("clamp: minVal(%f) > maxVal(%f), the result is undefined."), MinVal, MaxVal);
					}
				}
				else if (ResultType->GetKind() == SpvTypeKind::Integer)
				{
					if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
					{
						int MinVal = *(int*)(State.MinVal.GetData());
						int MaxVal = *(int*)(State.MaxVal.GetData());
						if (MinVal > MaxVal)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("clamp: minVal(%d) > maxVal(%d), the result is undefined."), MinVal, MaxVal);
						}
					}
					else
					{
						uint32 MinVal = *(uint32*)(State.MinVal.GetData());
						uint32 MaxVal = *(uint32*)(State.MaxVal.GetData());
						if (MinVal > MaxVal)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("clamp: minVal(%d) > maxVal(%d), the result is undefined."), MinVal, MaxVal);
						}
					}
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_SmoothStep>(InState))
		{
			const auto& State = std::get<SpvDebugState_SmoothStep>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float e0 = *(float*)(State.Edge0.GetData() + i * 4);
					float e1 = *(float*)(State.Edge1.GetData() + i * 4);
					if (e0 >= e1)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("smoothstep: Edge0(%f) >= Edge1(%f) for component %d, the result is undefined."), e0, e1, i);
					}
				}
			}
			else
			{
				float e0 = *(float*)(State.Edge0.GetData());
				float e1 = *(float*)(State.Edge1.GetData());
				if (e0 >= e1)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("smoothstep: Edge0(%f) >= Edge1(%f), the result is undefined."), e0, e1);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Div>(InState))
		{
			const auto& State = std::get<SpvDebugState_Div>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					if (dynamic_cast<SpvFloatType*>(ResultType))
					{
						float Operand2 = *(float*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("Division by zero is undefined for component %d."), i);
						}
					}
					else if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
					{
						int Operand2 = *(int*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("Division by zero is undefined for component %d."), i);
						}
					}
					else
					{
						uint32 Operand2 = *(uint32*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("Division by zero is undefined for component %d."), i);
						}
					}
				}
			}
			else
			{
				TArray<uint8> ZeroBuffer;
				ZeroBuffer.SetNumZeroed(State.Operand2.Num());
				if (FMemory::Memcmp(State.Operand2.GetData(), ZeroBuffer.GetData(), ZeroBuffer.Num()) == 0)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("Division by zero is undefined."));
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_ConvertF>(InState))
		{
			const auto& State = std::get<SpvDebugState_ConvertF>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float f = *(float*)(State.FloatValue.GetData() + i * 4);
					if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
					{
						if (int64(f) < int64(std::numeric_limits<int32>::min()) || int64(f) > int64(std::numeric_limits<int32>::max()))
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("Conversion(float to int) is undefined for component %d: It's not wide enough to hold the converted value(%f)."), i, f);
						}
					}
					else
					{
						if (f < 0.0f || uint64(f) > uint64(std::numeric_limits<uint32>::max()))
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("Conversion(float to uint) is undefined for component %d: It's not wide enough to hold the converted value(%f)."), i, f);
						}
					}
				}
			}
			else if(ResultType->GetKind() == SpvTypeKind::Integer)
			{
				float f = *(float*)(State.FloatValue.GetData());
				if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
				{
					if (int64(f) < int64(std::numeric_limits<int32>::min()) || int64(f) > int64(std::numeric_limits<int32>::max()))
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Conversion(float to int) is undefined: It's not wide enough to hold the converted value(%f)."), f);
					}
				}
				else
				{
					if (f < 0.0f || uint64(f) > uint64(std::numeric_limits<uint32>::max()))
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Conversion(float to uint) is undefined: It's not wide enough to hold the converted value(%f)."), f);
					}
				}
				
			}
		}
		else if (std::holds_alternative<SpvDebugState_Remainder>(InState))
		{
			const auto& State = std::get<SpvDebugState_Remainder>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					if (ResultType->GetKind() == SpvTypeKind::Float)
					{
						float Operand2 = *(float*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("float remainder by zero is undefined for component %d."), i);
						}
					}
					else if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
					{
						int32 Operand1 = *(int32*)(State.Operand1.GetData() + i * 4);
						int32 Operand2 = *(int32*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("int remainder by zero is undefined for component %d."), i);
						}
						else if (Operand2 == -1 && Operand1 == std::numeric_limits<int32>::min())
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("The remainder causing signed overflow is undefined for component %d."), i);
						}
					}
					else
					{
						uint32 Operand2 = *(uint32*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLineNumber = State.Line - ExtraLineNum;
							SH_LOG(LogShader, Error, TEXT("uint remainder by zero is undefined for component %d."), i);
						}
					}
				}
			}
			else
			{
				if (ResultType->GetKind() == SpvTypeKind::Float)
				{
					float Operand2 = *(float*)(State.Operand2.GetData());
					if (Operand2 == 0)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("float remainder by zero is undefined."));
					}
				}
				else if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
				{
					int32 Operand1 = *(int32*)(State.Operand1.GetData());
					int32 Operand2 = *(int32*)(State.Operand2.GetData());
					if (Operand2 == 0)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("int remainder by zero is undefined."));
					}
					else if (Operand2 == -1 && Operand1 == std::numeric_limits<int32>::min())
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("The remainder causing signed overflow is undefined."));
					}
				}
				else
				{
					uint32 Operand2 = *(uint32*)(State.Operand2.GetData());
					if (Operand2 == 0)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("uint remainder by zero is undefined."));
					}
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Log>(InState))
		{
			const auto& State = std::get<SpvDebugState_Log>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float X = *(float*)(State.X.GetData() + i * 4);
					if (X <= 0.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("log: x(%f) <= 0 for component %d, may produce an indeterminate result."), X, i);
					}
				}
			}
			else
			{
				float X = *(float*)(State.X.GetData());
				if (X <= 0.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("log: x(%f) <= 0, may produce an indeterminate result."), X);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Asin>(InState))
		{
			const auto& State = std::get<SpvDebugState_Asin>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float X = *(float*)(State.X.GetData() + i * 4);
					if (FMath::Abs(X) > 1.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Asin: abs x(%f) > 1 for component %d, may produce an indeterminate result."), X, i);
					}
				}
			}
			else
			{
				float X = *(float*)(State.X.GetData());
				if (FMath::Abs(X) > 1.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("Asin: abs x(%f) > 1, may produce an indeterminate result."), X);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Acos>(InState))
		{
			const auto& State = std::get<SpvDebugState_Acos>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float X = *(float*)(State.X.GetData() + i * 4);
					if (FMath::Abs(X) > 1.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Acos: abs x(%f) > 1 for component %d, may produce an indeterminate result."), X, i);
					}
				}
			}
			else
			{
				float X = *(float*)(State.X.GetData());
				if (FMath::Abs(X) > 1.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("Acos: abs x(%f) > 1, may produce an indeterminate result."), X);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Sqrt>(InState))
		{
			const auto& State = std::get<SpvDebugState_Sqrt>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float X = *(float*)(State.X.GetData() + i * 4);
					if (X < 0.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Sqrt: x(%f) < 0 for component %d, may produce an indeterminate result."), X, i);
					}
				}
			}
			else
			{
				float X = *(float*)(State.X.GetData());
				if (X < 0.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("Sqrt: x(%f) < 0, may produce an indeterminate result."), X);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_InverseSqrt>(InState))
		{
			const auto& State = std::get<SpvDebugState_InverseSqrt>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float X = *(float*)(State.X.GetData() + i * 4);
					if (X <= 0.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("InverseSqrt: x(%f) <= 0 for component %d, may produce an indeterminate result."), X, i);
					}
				}
			}
			else
			{
				float X = *(float*)(State.X.GetData());
				if (X <= 0.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("InverseSqrt: x(%f) <= 0, may produce an indeterminate result."), X);
				}
			}
		}
		else if (std::holds_alternative<SpvDebugState_Atan2>(InState))
		{
			const auto& State = std::get<SpvDebugState_Atan2>(InState);
			SpvType* ResultType = DebuggerContext->Types[State.ResultType].Get();
			if (ResultType->GetKind() == SpvTypeKind::Vector)
			{
				int ElemCount = static_cast<SpvVectorType*>(ResultType)->ElementCount;
				for (int i = 0; i < ElemCount; i++)
				{
					float Y = *(float*)(State.Y.GetData() + i * 4);
					float X = *(float*)(State.X.GetData() + i * 4);
					if (Y == 0.0f && X == 0.0f)
					{
						Error = "Undefined";
						StopLineNumber = State.Line - ExtraLineNum;
						SH_LOG(LogShader, Error, TEXT("Atan2: y = x = 0 for component %d, the result is undefined."), i);
					}
				}
			}
			else
			{
				float Y = *(float*)(State.Y.GetData());
				float X = *(float*)(State.X.GetData());
				if (Y == 0.0f && X == 0.0f)
				{
					Error = "Undefined";
					StopLineNumber = State.Line - ExtraLineNum;
					SH_LOG(LogShader, Error, TEXT("Atan2: y = x = 0, the result is undefined."));
				}
			}
		}
	}

	void ShaderDebugger::InvokePixel(bool GlobalValidation)
	{
		const auto& PsInvocation = std::get<PixelState>(Invocation);

		DebugShader = ShaderEditor->CreateGpuShader();
		DebugShader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		ExtraArgs.Add("-ignore-validation-error");

		FString ErrorInfo, WarnInfo;
		if (!GGpuRhi->CompileShader(DebugShader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString();
			SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo, *ErrorInfo);
			throw std::runtime_error(TCHAR_TO_UTF8(*FailureInfo));
		}

		uint32 BufferSize = GlobalValidation ? 1024 : 1024 * 1024 * 2;
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
		SetupBinding(PsInvocation.Builders.GlobalBuilder);
		SetupBinding(PsInvocation.Builders.PassBuilder);
		SetupBinding(PsInvocation.Builders.ShaderBuilder);

		auto PixelDebuggerContext = MakeUnique<SpvPixelDebuggerContext>(PixelCoord, SpvBindings);

		SpirvParser Parser;
		Parser.Parse(DebugShader->SpvCode);
		SpvMetaVisitor MetaVisitor{ *PixelDebuggerContext };
		Parser.Accept(&MetaVisitor);
		TArray<uint32> PatchedSpv;
		if (GlobalValidation)
		{
			SpvValidator Validator{ *PixelDebuggerContext, bEnableUbsan, ShaderType::PixelShader };
			Parser.Accept(&Validator);
			Validator.GetPatcher().Dump(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Validation.spvasm");
			PatchedSpv = Validator.GetPatcher().GetSpv();
		}
		else
		{
			SpvPixelDebuggerVisitor DebuggerVisitor{ *PixelDebuggerContext, bEnableUbsan };
			Parser.Accept(&DebuggerVisitor);
			DebuggerVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Patched.spvasm");
			PatchedSpv = DebuggerVisitor.GetPatcher().GetSpv();
		}

		auto EntryPoint = StringCast<UTF8CHAR>(*DebugShader->GetEntryPoint());
		ShaderConductor::Compiler::TargetDesc HlslTargetDesc{};
		HlslTargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
		HlslTargetDesc.version = "60";
		ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile({}, { PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 }, (char*)EntryPoint.Get(),
			ShaderConductor::ShaderStage::PixelShader, HlslTargetDesc);
		FString PatchedHlsl = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };
		if (GlobalValidation)
		{
			FFileHelper::SaveStringToFile(PatchedHlsl, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Validation.hlsl"));
		}
		else
		{
			FFileHelper::SaveStringToFile(PatchedHlsl, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Patched.hlsl"));
		}

		if (ShaderResultDesc.hasError)
		{
			FString ErrorInfo = static_cast<const char*>(ShaderResultDesc.errorWarningMsg.Data());
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString();
			SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo, *ErrorInfo);
			throw std::runtime_error(TCHAR_TO_UTF8(*FailureInfo));
		}

		CurDebugStateIndex = 0;
		DebuggerContext = MoveTemp(PixelDebuggerContext);
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
			.Vs = PsInvocation.PipelineDesc.Vs,
			.Ps = PatchedShader,
			.RasterizerState = PsInvocation.PipelineDesc.RasterizerState,
			.Primitive = PsInvocation.PipelineDesc.Primitive
		};
		PatchedBindings.ApplyBindGroupLayout(PatchedPipelineDesc);
		TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PatchedPipelineDesc);

		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			GpuResourceHelper::ClearRWResource(CmdRecorder, DebugBuffer);
			CmdRecorder->Barriers({ GpuBarrierInfo{.Resource = DebugBuffer, .NewState = GpuResourceState::UnorderedAccess} });
			auto PassRecorder = CmdRecorder->BeginRenderPass({}, TEXT("Debugger"));
			{
				PassRecorder->SetViewPort(PsInvocation.ViewPortDesc);
				PassRecorder->SetRenderPipelineState(Pipeline);
				PatchedBindings.ApplyBindGroup(PassRecorder);
				PsInvocation.DrawFunction(PassRecorder);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
	}

	void ShaderDebugger::DebugPixel(const Vector2u& InPixelCoord, const InvocationState& InState)
	{
		PixelCoord = InPixelCoord;
		Invocation = InState;
		bEnableUbsan = EnableUbsan();
		
		InvokePixel();
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

	std::optional<Vector2u> ShaderDebugger::ValidatePixel(const InvocationState& InState)
	{
		Invocation = InState;
		bEnableUbsan = EnableUbsan();

		InvokePixel(true);

		std::optional<Vector2u> ErrorCoord;
		uint8* DebugBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebugBuffer, GpuResourceMapMode::Read_Only);
		uint32 Offset = *(uint32*)(DebugBufferData);
		if (Offset > 0)
		{
			ErrorCoord = *(Vector2u*)(DebugBufferData + 4);
		}
		GGpuRhi->UnMapGpuBuffer(DebugBuffer);

		return ErrorCoord;
	}

	void ShaderDebugger::DebugCompute(const InvocationState& InState)
	{

	}

	void ShaderDebugger::Reset()
	{
		CurValidLine.reset();
		CallStack.Empty();
		DebuggerContext = nullptr;
		Scope = nullptr;
		ActiveCallStackScope = nullptr;
		AssertResult = nullptr;
		DebuggerError.Empty();
		DirtyVars.Empty();
		StopLineNumber = 0;
		ReturnValue.Empty();
		DebugStates.Reset();
		SortedVariableDescs.clear();
		SpvBindings.Empty();
		DebugShader = nullptr;
		DebugBuffer = nullptr;
		bEnableUbsan = false;

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
		DebuggerLocalVariableView->SetOnShowUninitialized([this](bool bShowUninitialized) { ShowDeuggerVariable(ActiveCallStackScope); });
		DebuggerGlobalVariableView->SetOnShowUninitialized([this](bool bShowUninitialized) { ShowDeuggerVariable(ActiveCallStackScope); });

		DebuggerCallStackView->OnSelectionChanged = [this, DebuggerWatchView](const FString& FuncName) {
			int32 ExtraLineNum = ShaderEditor->GetShaderAsset()->GetExtraLineNum();
			if (GetFunctionSig(GetFunctionDesc(Scope)) == FuncName)
			{
				StopLineNumber = CurValidLine.value() - ExtraLineNum;
				ShowDeuggerVariable(Scope);
				ActiveCallStackScope = Scope;
			}
			else if (auto CallPoint = CallStack.FindByPredicate([this, FuncName](const auto& InItem) {
				if (GetFunctionSig(GetFunctionDesc(InItem.Scope)) == FuncName)
				{
					return true;
				}
				return false;
			}))
			{
				StopLineNumber = CallPoint->Line - ExtraLineNum;
				ShowDeuggerVariable(CallPoint->Scope);
				ActiveCallStackScope = CallPoint->Scope;
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
				TArray<int32> Indexes = { (int32*)(DebuggerData + Offset), IndexNum};
				Offset += IndexNum * 4;
				int32 ValueSize = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				TArray<uint8> DirtyValue = {DebuggerData + Offset, ValueSize};
				Offset += DirtyValue.Num();

				SpvVariable* DirtyVar = DebuggerContext->FindVar(VarId);
				auto AccessOrError = GetAccess(DirtyVar, Indexes);
				if (!AccessOrError.HasError())
				{
					const auto& [_, ByteOffset] = AccessOrError.GetValue();
					TArray<uint8> PreDirtyValue = { &DirtyVar->GetBuffer()[ByteOffset], ValueSize };

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
				}
				else
				{
					auto NewDebugState = SpvDebugState_VarChange{
						.Line = Line,
						.Error = AccessOrError.GetError()
					};
					DebugStates.Add(MoveTemp(NewDebugState));
				}

				break;
			}
			case SpvDebuggerStateType::ScopeChange:
			{
				SpvId ScopeId = *(SpvId*)(DebuggerData + Offset);
				Offset += 4;
				SpvLexicalScope* NewScope = DebuggerContext->LexicalScopes[ScopeId].Get();
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
			case SpvDebuggerStateType::Access:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId VarId = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 IndexNum = *(int32*)(DebuggerData + Offset); Offset += 4;
				TArray<int32> Indexes = { (int32*)(DebuggerData + Offset), IndexNum }; Offset += IndexNum * 4;
				DebugStates.Add(SpvDebugState_Access{
					.Line = Line,
					.VarId = VarId,
					.Indexes = MoveTemp(Indexes)
				});
				break;
			}
			case SpvDebuggerStateType::Normalize:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_Normalize{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::SmoothStep:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Edge0 = { DebuggerData + Offset, Size }; Offset += Edge0.Num();
				TArray<uint8> Edge1 = { DebuggerData + Offset, Size }; Offset += Edge1.Num();
				DebugStates.Add(SpvDebugState_SmoothStep{
					.Line = Line,
					.ResultType = ResultType,
					.Edge0 = MoveTemp(Edge0),
					.Edge1 = MoveTemp(Edge1)
				});
				break;
			}
			case SpvDebuggerStateType::Pow:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				TArray<uint8> Y = { DebuggerData + Offset, Size }; Offset += Y.Num();
				DebugStates.Add(SpvDebugState_Pow{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X),
					.Y = MoveTemp(Y)
				});
				break;
			}
			case SpvDebuggerStateType::Clamp:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> MinVal = { DebuggerData + Offset, Size }; Offset += MinVal.Num();
				TArray<uint8> MaxVal = { DebuggerData + Offset, Size }; Offset += MaxVal.Num();
				DebugStates.Add(SpvDebugState_Clamp{
					.Line = Line,
					.ResultType = ResultType,
					.MinVal = MoveTemp(MinVal),
					.MaxVal = MoveTemp(MaxVal)
				});
				break;
			}
			case SpvDebuggerStateType::Div:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Operand2 = { DebuggerData + Offset, Size }; Offset += Operand2.Num();
				DebugStates.Add(SpvDebugState_Div{
					.Line = Line,
					.ResultType = ResultType,
					.Operand2 = MoveTemp(Operand2),
				});
				break;
			}
			case SpvDebuggerStateType::ConvertF:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> FloatValue = { DebuggerData + Offset, Size }; Offset += FloatValue.Num();
				DebugStates.Add(SpvDebugState_ConvertF{
					.Line = Line,
					.ResultType = ResultType,
					.FloatValue = MoveTemp(FloatValue),
				});
				break;
			}
			case SpvDebuggerStateType::Remainder:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Operand1 = { DebuggerData + Offset, Size }; Offset += Operand1.Num();
				TArray<uint8> Operand2 = { DebuggerData + Offset, Size }; Offset += Operand2.Num();
				DebugStates.Add(SpvDebugState_Remainder{
					.Line = Line,
					.ResultType = ResultType,
					.Operand1 = MoveTemp(Operand1),
					.Operand2 = MoveTemp(Operand2)
				});
				break;
			}
			case SpvDebuggerStateType::Log:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_Log{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Asin:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_Asin{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Acos:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_Acos{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Sqrt:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_Sqrt{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::InverseSqrt:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_InverseSqrt{
					.Line = Line,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Atan2:
			{
				int32 Line = *(int32*)(DebuggerData + Offset); Offset += 4;
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Y = { DebuggerData + Offset, Size }; Offset += Y.Num();
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				DebugStates.Add(SpvDebugState_Atan2{
					.Line = Line,
					.ResultType = ResultType,
					.Y = MoveTemp(Y),
					.X = MoveTemp(X)
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
				int32 Line = *(int32*)(DebuggerData + Offset);  Offset += 4;
				SpvId CallId = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				DebugStates.Add(SpvDebugState_FuncCall{
					.Line = Line,
					.CallId = CallId,
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
