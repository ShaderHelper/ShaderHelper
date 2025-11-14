#include "CommonHeader.h"
#include "ShaderDebugger.h"
#include "ShaderConductor.hpp"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "RenderResource/Shader/Shader.h"
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
							bool VisibleScope = VarDesc->Parent->Contains(CallStackScope);
							bool VisibleLine = VarDesc->Line < StopLineNumber + ExtraLineNum && VarDesc->Line > ExtraLineNum;
							if (VisibleScope && VisibleLine)
							{
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
				VisibleLocalVarBindings += "StructuredBuffer<__Expression_Vars_Set> __Expression_Vars : register(t114514, space0);\n";

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
			R"(
RWByteAddressBuffer _DebuggerBuffer : register(u465, space0);
template<typename T>
void __Expression_Output(T __Expression_Result) 
{
	_DebuggerBuffer.Store<T>(0, __Expression_Result);
}
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
			SpvExprDebuggerContext ExprContext;
			SpirvParser Parser;
			Parser.Parse(Shader->SpvCode);
			SpvMetaVisitor MetaVisitor{ ExprContext };
			Parser.Accept(&MetaVisitor);
			SpvExprDebuggerVisitor ExprVisitor{ ExprContext };
			Parser.Accept(&ExprVisitor);

			if (ExprContext.ResultTypeDesc)
			{
				FText TypeName = LOCALIZATION("InvalidExpr");
				TypeName = FText::FromString(GetTypeDescStr(ExprContext.ResultTypeDesc));

				BindingContext Bindings;
				int32 ResultSize = GetTypeByteSize(ExprContext.ResultTypeDesc);
				TArray<uint8> Datas;
				Datas.SetNumZeroed(ResultSize);
				TRefCountPtr<GpuBuffer> DebuggerBuffer = GGpuRhi->CreateBuffer({
					.ByteSize = (uint32)ResultSize,
					.Usage = GpuBufferUsage::RWRaw,
					.InitialData = Datas,
				});

				TRefCountPtr<GpuBuffer> LocalVarsInput = GGpuRhi->CreateBuffer({
					.ByteSize = (uint32)InputData.Num(),
					.Usage = GpuBufferUsage::Structured,
					.InitialData = InputData,
					.StructuredInit = {
						.Stride = (uint32)InputData.Num()
					}
				});

				GpuBindGroupDesc BindGroupDesc = (*BindingBuilders.GlobalBuilder).BingGroupBuilder.GetDesc();
				GpuBindGroupLayoutDesc LayoutDesc = (*BindingBuilders.GlobalBuilder).LayoutBuilder.GetDesc();
				BindGroupDesc.Resources.Add(0721, { DebuggerBuffer });
				LayoutDesc.Layouts.Add(0721, { BindingType::RWRawBuffer });
				BindGroupDesc.Resources.Add(114514, { LocalVarsInput });
				LayoutDesc.Layouts.Add(114514, { BindingType::StructuredBuffer });
				TRefCountPtr<GpuBindGroupLayout> PatchedBindGroupLayout = GGpuRhi->CreateBindGroupLayout(LayoutDesc);
				BindGroupDesc.Layout = PatchedBindGroupLayout;
				TRefCountPtr<GpuBindGroup> PacthedBindGroup = GGpuRhi->CreateBindGroup(BindGroupDesc);
				Bindings.SetGlobalBindGroup(PacthedBindGroup);
				Bindings.SetGlobalBindGroupLayout(PatchedBindGroupLayout);

				if (BindingBuilders.PassBuilder)
				{
					Bindings.SetPassBindGroup(BindingBuilders.PassBuilder.value().BingGroupBuilder.Build());
					Bindings.SetPassBindGroupLayout(BindingBuilders.PassBuilder.value().LayoutBuilder.Build());
				}
				if (BindingBuilders.ShaderBuilder)
				{
					Bindings.SetShaderBindGroup(BindingBuilders.ShaderBuilder.value().BingGroupBuilder.Build());
					Bindings.SetShaderBindGroupLayout(BindingBuilders.ShaderBuilder.value().LayoutBuilder.Build());
				}

				if (Shader->GetShaderType() == ShaderType::PixelShader)
				{
					Shader->CompilerFlag = GpuShaderCompilerFlag::None;
					if (GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo, ExtraArgs))
					{
						GpuRenderPipelineStateDesc PatchedPipelineDesc{
						.Vs = PsState.value().PipelineDesc.Vs,
						.Ps = Shader,
						.RasterizerState = PsState.value().PipelineDesc.RasterizerState,
						.Primitive = PsState.value().PipelineDesc.Primitive
						};
						Bindings.ApplyBindGroupLayout(PatchedPipelineDesc);
						TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PatchedPipelineDesc);
						auto CmdRecorder = GGpuRhi->BeginRecording();
						{
							auto PassRecorder = CmdRecorder->BeginRenderPass({}, TEXT("Debugger"));
							{
								PassRecorder->SetViewPort(PsState.value().ViewPortDesc);
								PassRecorder->SetRenderPipelineState(Pipeline);
								Bindings.ApplyBindGroup(PassRecorder);
								PsState.value().DrawFunction(PassRecorder);
							}
							CmdRecorder->EndRenderPass(PassRecorder);
						}
						GGpuRhi->EndRecording(CmdRecorder);
						GGpuRhi->Submit({ CmdRecorder });

						uint8* DebuggerBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebuggerBuffer, GpuResourceMapMode::Read_Only);
						TArray<uint8> ResultValue = { DebuggerBufferData, ResultSize };
						GGpuRhi->UnMapGpuBuffer(DebuggerBuffer);

						TArray<Vector2i> ResultRange;
						ResultRange.Add({ 0, ResultValue.Num() });
						FString ValueStr = GetValueStr(ResultValue, ExprContext.ResultTypeDesc, ResultRange, 0);

						TArray<TSharedPtr<ExpressionNode>> Children;
						if (ExprContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Composite || ExprContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Array
							|| ExprContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
						{
							Children = AppendExprChildNodes(ExprContext.ResultTypeDesc, ResultRange, ResultValue, 0);
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
		TRefCountPtr<GpuShader> Shader = ShaderEditor->CreateGpuShader();
		Shader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");

		FString ErrorInfo, WarnInfo;
		if (!GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			FString FailureInfo = LOCALIZATION("DebugFailure").ToString();
			SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo, *ErrorInfo);
			throw std::runtime_error(TCHAR_TO_UTF8(*FailureInfo));
		}

		ShaderTU TU{ Shader->GetSourceText(), Shader->GetIncludeDirs() };
		Funcs = TU.GetFuncs();

		TArray<SpvBinding> Bindings;
		BindingContext PatchedBindings;
		uint32 BufferSize = 1024 * 1024 * 2;
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
		SetupBinding(InBuilders.GlobalBuilder);
		SetupBinding(InBuilders.PassBuilder);
		SetupBinding(InBuilders.ShaderBuilder);

		PixelDebuggerContext = SpvPixelDebuggerContext{InState.Coord, Funcs, Bindings };

		SpirvParser Parser;
		Parser.Parse(Shader->SpvCode);
		SpvMetaVisitor MetaVisitor{ PixelDebuggerContext.value() };
		Parser.Accept(&MetaVisitor);
		SpvPixelDebuggerVisitor PixelDebuggerVisitor{ PixelDebuggerContext.value(), EnableUbsan()};
		Parser.Accept(&PixelDebuggerVisitor);
#if !SH_SHIPPING
		PixelDebuggerVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / Shader->GetShaderName() / Shader->GetShaderName() + "Patched.spvasm");
#endif 

		const TArray<uint32>& PatchedSpv = PixelDebuggerVisitor.GetPatcher().GetSpv();
		auto EntryPoint = StringCast<UTF8CHAR>(*Shader->GetEntryPoint());
		ShaderConductor::Compiler::TargetDesc HlslTargetDesc{};
		HlslTargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
		HlslTargetDesc.version = "60";
		ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile({ PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 }, (char*)EntryPoint.Get(),
			ShaderConductor::ShaderStage::PixelShader, HlslTargetDesc);
		FString PatchedHlsl = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };
#if !SH_SHIPPING
		FFileHelper::SaveStringToFile(PatchedHlsl, *(PathHelper::SavedShaderDir() / Shader->GetShaderName() / Shader->GetShaderName() + "Patched.hlsl"));
#endif
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
			.Type = Shader->GetShaderType(),
			.EntryPoint = Shader->GetEntryPoint()
		});
		if (!GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo))
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
		BindingBuilders = {};

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
