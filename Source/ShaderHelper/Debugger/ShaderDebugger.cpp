#include "CommonHeader.h"
#include "ShaderDebugger.h"
#include "ShaderConductor.hpp"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/Spirv/SpirvExprDebugger.h"
#include "GpuApi/Spirv/SpirvPixelPreviewer.h"
#include "GpuApi/Spirv/SpirvValidator.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "Renderer/RenderGraph.h"

using namespace FW;

namespace SH
{
	
	static void DumpDebugStatesToFile(const TArray<SpvDebugState>& InDebugStates, const SpvDebuggerContext* InContext, const FString& File)
	{
	   FString Dump;
	   Dump += FString::Printf(TEXT("=== Debug States Dump (%d states) ===\n\n"), InDebugStates.Num());

	   for (int32 i = 0; i < InDebugStates.Num(); i++)
	   {
		   const SpvDebugState& State = InDebugStates[i];
		   Dump += FString::Printf(TEXT("[%d] "), i);

		   if (std::holds_alternative<SpvDebugState_VarChange>(State))
		   {
			   const auto& S = std::get<SpvDebugState_VarChange>(State);
			   FString SourceFile = InContext->GetSourceFileName(S.Source);
			   FString VarName = InContext->Names.contains(S.Change.VarId) ? InContext->Names.at(S.Change.VarId) : FString::Printf(TEXT("%d"), S.Change.VarId.GetValue());
			   Dump += FString::Printf(TEXT("VarChange  Line=%d Source=%s Var=%s"), S.Line, *SourceFile, *VarName);
			   if (!S.Error.IsEmpty())
			   {
				   Dump += FString::Printf(TEXT(" Error=\"%s\""), *S.Error);
			   }
		   }
		   else if (std::holds_alternative<SpvDebugState_ScopeChange>(State))
		   {
			   const auto& S = std::get<SpvDebugState_ScopeChange>(State);
			   Dump += FString::Printf(TEXT("ScopeChange  Pre=%d New=%d"), S.Change.PreScope ? S.Change.PreScope->GetId().GetValue() : 0, S.Change.NewScope ? S.Change.NewScope->GetId().GetValue() : 0);
		   }
		   else if (std::holds_alternative<SpvDebugState_ReturnValue>(State))
		   {
			   const auto& S = std::get<SpvDebugState_ReturnValue>(State);
			   Dump += FString::Printf(TEXT("ReturnValue  Line=%d Source=%s"), S.Line, *InContext->GetSourceFileName(S.Source));
		   }
		   else if (std::holds_alternative<SpvDebugState_FuncCall>(State))
		   {
			   const auto& S = std::get<SpvDebugState_FuncCall>(State);
			   Dump += FString::Printf(TEXT("FuncCall  Line=%d Source=%s CallId=%d"), S.Line, *InContext->GetSourceFileName(S.Source), S.CallId.GetValue());
		   }
		   else if (std::holds_alternative<SpvDebugState_Tag>(State))
		   {
			   const auto& S = std::get<SpvDebugState_Tag>(State);
			   Dump += FString::Printf(TEXT("Tag  Line=%d Source=%s"), S.Line, *InContext->GetSourceFileName(S.Source));
			   if (S.bCondition) Dump += TEXT(" Condition");
			   if (S.bFuncCallAfterReturn) Dump += TEXT(" FuncCallAfterReturn");
			   if (S.bReturn) Dump += TEXT(" Return");
			   if (S.bKill) Dump += TEXT(" Kill");
		   }
		   else if (std::holds_alternative<SpvDebugState_Access>(State))
		   {
			   const auto& S = std::get<SpvDebugState_Access>(State);
			   FString VarName = InContext->Names.contains(S.VarId) ? InContext->Names.at(S.VarId) : FString::Printf(TEXT("%d"), S.VarId.GetValue());
			   Dump += FString::Printf(TEXT("Access  Line=%d Source=%s Var=%s"), S.Line, *InContext->GetSourceFileName(S.Source), *VarName);
		   }
		   else
		   {
			   struct { int32 Line; SpvId Source; const TCHAR* TypeName; } Info{};
			   if (std::holds_alternative<SpvDebugState_Normalize>(State))       { const auto& S = std::get<SpvDebugState_Normalize>(State);    Info = {S.Line, S.Source, TEXT("Normalize")}; }
			   else if (std::holds_alternative<SpvDebugState_SmoothStep>(State)) { const auto& S = std::get<SpvDebugState_SmoothStep>(State);   Info = {S.Line, S.Source, TEXT("SmoothStep")}; }
			   else if (std::holds_alternative<SpvDebugState_Pow>(State))        { const auto& S = std::get<SpvDebugState_Pow>(State);           Info = {S.Line, S.Source, TEXT("Pow")}; }
			   else if (std::holds_alternative<SpvDebugState_Clamp>(State))      { const auto& S = std::get<SpvDebugState_Clamp>(State);         Info = {S.Line, S.Source, TEXT("Clamp")}; }
			   else if (std::holds_alternative<SpvDebugState_Div>(State))        { const auto& S = std::get<SpvDebugState_Div>(State);           Info = {S.Line, S.Source, TEXT("Div")}; }
			   else if (std::holds_alternative<SpvDebugState_ConvertF>(State))   { const auto& S = std::get<SpvDebugState_ConvertF>(State);      Info = {S.Line, S.Source, TEXT("ConvertF")}; }
			   else if (std::holds_alternative<SpvDebugState_Remainder>(State))  { const auto& S = std::get<SpvDebugState_Remainder>(State);     Info = {S.Line, S.Source, TEXT("Remainder")}; }
			   else if (std::holds_alternative<SpvDebugState_Log>(State))        { const auto& S = std::get<SpvDebugState_Log>(State);           Info = {S.Line, S.Source, TEXT("Log")}; }
			   else if (std::holds_alternative<SpvDebugState_Asin>(State))       { const auto& S = std::get<SpvDebugState_Asin>(State);          Info = {S.Line, S.Source, TEXT("Asin")}; }
			   else if (std::holds_alternative<SpvDebugState_Acos>(State))       { const auto& S = std::get<SpvDebugState_Acos>(State);          Info = {S.Line, S.Source, TEXT("Acos")}; }
			   else if (std::holds_alternative<SpvDebugState_Sqrt>(State))       { const auto& S = std::get<SpvDebugState_Sqrt>(State);          Info = {S.Line, S.Source, TEXT("Sqrt")}; }
			   else if (std::holds_alternative<SpvDebugState_InverseSqrt>(State)){ const auto& S = std::get<SpvDebugState_InverseSqrt>(State);   Info = {S.Line, S.Source, TEXT("InverseSqrt")}; }
			   else if (std::holds_alternative<SpvDebugState_Atan2>(State))      { const auto& S = std::get<SpvDebugState_Atan2>(State);         Info = {S.Line, S.Source, TEXT("Atan2")}; }
			   Dump += FString::Printf(TEXT("%s  Line=%d Source=%s"), Info.TypeName, Info.Line, *InContext->GetSourceFileName(Info.Source));
		   }

		   Dump += TEXT("\n");
	   }

	   FFileHelper::SaveStringToFile(Dump, *File);
	}

	TArray<ExpressionNodePtr> AppendChildNodes(GpuShaderLanguage Lang, SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, const TArray<SpvVarDirtyRange>& DirtyRanges, const TArray<uint8>& Value, int32 InOffset)
	{
		TArray<ExpressionNodePtr> Nodes;
		int32 Offset = InOffset;
		
		// Helper lambda to check if a range is dirty
		auto CheckDirty = [&DirtyRanges](int32 ByteOffset, int32 ByteSize) -> bool {
			if (DirtyRanges.IsEmpty()) return false;
			for (const auto& Range : DirtyRanges)
			{
				int32 RangeStartOffset = Range.ByteOffset;
				int32 RangeEndOffset = Range.ByteOffset + Range.ByteSize;
				if ((RangeStartOffset >= ByteOffset && RangeStartOffset < ByteOffset + ByteSize) ||
					(RangeEndOffset > ByteOffset && RangeEndOffset <= ByteOffset + ByteSize) ||
					(RangeStartOffset < ByteOffset && RangeEndOffset > ByteOffset + ByteSize))
				{
					return true;
				}
			}
			return false;
		};

		if (TypeDesc->GetKind() == SpvTypeDescKind::Member)
		{
			SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(TypeDesc);
			int32 MemberByteSize = GetTypeByteSize(MemberTypeDesc);
			FString ValueStr = GetValueStr(Value, MemberTypeDesc->GetTypeDesc(), InitializedRanges, Offset, DebuggerViewHex);
			FString TypeName = GetTypeDescStr(MemberTypeDesc->GetTypeDesc(), Lang);
			auto Data = MakeShared<ExpressionNode>(MemberTypeDesc->GetName(), MoveTemp(ValueStr), TypeName);
			if (MemberTypeDesc->GetTypeDesc()->GetKind() == SpvTypeDescKind::Composite)
			{
				Data->Children = AppendChildNodes(Lang, MemberTypeDesc->GetTypeDesc(), InitializedRanges, DirtyRanges, Value, Offset);
			}
			if (CheckDirty(InOffset, MemberByteSize))
			{
				Data->Dirty = true;
			}
			Nodes.Add(MoveTemp(Data));
		}
		else if (TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(TypeDesc);
			for (SpvTypeDesc* MemberTypeDesc : CompositeTypeDesc->GetMemberTypeDescs())
			{
				Nodes.Append(AppendChildNodes(Lang, MemberTypeDesc, InitializedRanges, DirtyRanges, Value, Offset));
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
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset, DebuggerViewHex);
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				FString TypeName = GetTypeDescStr(ElementTypeDesc, Lang);
				auto Data = MakeShared<ExpressionNode>(MemberName, MoveTemp(ValueStr), TypeName);
				if (CheckDirty(Offset, ElementTypeSize))
				{
					Data->Dirty = true;
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
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset, DebuggerViewHex);
				FString TypeName = GetTypeDescStr(ElementTypeDesc, Lang);
				auto Data = MakeShared<ExpressionNode>(MemberName, MoveTemp(ValueStr), TypeName);
				Data->Children = AppendChildNodes(Lang, ElementTypeDesc, InitializedRanges, DirtyRanges, Value, Offset);
				if (CheckDirty(Offset, ElementTypeSize))
				{
					Data->Dirty = true;
				}
				Nodes.Add(MoveTemp(Data));
				Offset += ElementTypeSize;
			}
		}
		return Nodes;
	}

	ExpressionNode ShaderDebugger::EvaluateExpression(const FString& InExpression) const
	{
		// Check if expression is a simple variable identifier
		FString TrimmedExpr = InExpression.TrimStartAndEnd();
		if (!TrimmedExpr.IsEmpty() && DebuggerContext)
		{
			bool bIsSimpleIdentifier = true;
			for (int32 i = 0; i < TrimmedExpr.Len(); i++)
			{
				TCHAR Ch = TrimmedExpr[i];
				if (!FChar::IsIdentifier(Ch))
				{
					bIsSimpleIdentifier = false;
					break;
				}
			}

			if (bIsSimpleIdentifier)
			{
				// Try to find variable by name from SortedVariableDescs
				for (const auto& [VarId, VarDesc] : SortedVariableDescs)
				{
					//Note: FString opreator== case insensitive
					if (VarDesc->Name.Equals(TrimmedExpr))
					{
						if (SpvVariable* Var = DebuggerContext->FindVar(VarId))
						{
							if (!Var->IsExternal())
							{
								// Check if variable is visible in current scope
								bool VisibleScope = VarDesc->Parent->Contains(Scope);
								if (ActiveCallPoint)
								{
									VisibleScope = VarDesc->Parent->Contains(ActiveCallPoint.value().Scope);
								}
								if (VisibleScope)
								{
									FString TypeName = GetTypeDescStr(VarDesc->TypeDesc, DebugShader->GetShaderLanguage());
									const TArray<uint8>& Value = std::get<SpvObject::Internal>(Var->Storage).Value;
									if (!Value.IsEmpty())
									{
										TArray<Vector2i> InitializedRanges = { {0, Value.Num()} };
										FString ValueStr = GetValueStr(Value, VarDesc->TypeDesc, InitializedRanges, 0, DebuggerViewHex);

										TArray<SpvVarDirtyRange> Ranges;
										DirtyVars.MultiFind(VarId, Ranges);

										TArray<ExpressionNodePtr> Children;
										if (VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Composite || 
											VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Array ||
											VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
										{
											Children = AppendChildNodes(DebugShader->GetShaderLanguage(), VarDesc->TypeDesc, InitializedRanges, Ranges, Value, 0);
										}

										return { .Expr = TrimmedExpr, .ValueStr = ValueStr, .TypeName = TypeName,
											.Dirty = !Ranges.IsEmpty(), .IsVarIdentifier = true,
											.Children = MoveTemp(Children) };
									}
								}
							}
						}
					}
				}

				return { .Expr = InExpression, .ValueStr = LOCALIZATION("InvalidExpr").ToString(), .IsVarIdentifier = true };
			}
		}

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		ExtraArgs.Add("-ignore-validation-error");
		FString ErrorInfo, WarnInfo;
		GpuShaderLanguage Lang = DebugShader->GetShaderLanguage();

		if (DebugShader->GetShaderType() == ShaderType::PixelShader)
		{
			const auto& PsInvocation = std::get<PixelState>(Invocation);
			int32 DebugStateIndex = CurDebugStateIndex;
			if (ActiveCallPoint)
			{
				DebugStateIndex = ActiveCallPoint.value().DebugStateIndex;
			}
			// CPU-only states (ScopeChange, FuncCallAfterReturn) have no corresponding
			// SPIR-V instruction. Skip forward to the next GPU-visible state so the
			// ExprDebugger can find a valid insertion point.
			auto IsCpuOnlyState = [](const SpvDebugState& State) {
				if (std::holds_alternative<SpvDebugState_ScopeChange>(State))
					return true;
				if (auto* Tag = std::get_if<SpvDebugState_Tag>(&State))
					return Tag->bFuncCallAfterReturn;
				return false;
			};
			int32 ExprStateIndex = DebugStateIndex;
			while (ExprStateIndex < DebugStates.Num() && IsCpuOnlyState(DebugStates[ExprStateIndex]))
			{
				ExprStateIndex++;
			}
			// _DebugStateNum_ in ExprDebugger only counts GPU-side _Append*_ calls.
			// Subtract CPU-only state count to get the correct GPU-side index.
			int32 GpuStateIndex = ExprStateIndex;
			for (int32 i = 0; i < ExprStateIndex; i++)
			{
				if (IsCpuOnlyState(DebugStates[i]))
				{
					GpuStateIndex--;
				}
			}
			SpvPixelExprDebuggerContext ExprContext{ DebugStates[ExprStateIndex], GpuStateIndex,
				PixelCoord, SpvBindings };
			SpirvParser Parser;
			Parser.Parse(DebugShader->SpvCode);
			SpvMetaVisitor MetaVisitor{ ExprContext };
			Parser.Accept(&MetaVisitor);
			SpvPixelExprDebuggerVisitor ExprVisitor{ ExprContext, Lang, bEnableUbsan };
			Parser.Accept(&ExprVisitor);
			ExprVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "ExprPatched.spvasm");
			
			const TArray<uint32>& PatchedSpv = ExprVisitor.GetPatcher().GetSpv();
			
			ShaderConductor::Compiler::TargetDesc TargetDesc{};
			ShaderConductor::Compiler::Options Options{};
			Options.force_zero_initialized_variables = true;
			FString FileExtension;
			FString EntryPoint = DebugShader->GetEntryPoint();
			if (Lang == GpuShaderLanguage::HLSL)
			{
				TargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
				TargetDesc.version = "66";
				FileExtension = TEXT(".hlsl");
			}
			else
			{
				Options.vulkanSemantics = true;
				TargetDesc.language = ShaderConductor::ShadingLanguage::Glsl;
				TargetDesc.version = "450";
				FileExtension = TEXT(".glsl");
			}
			
			auto EntryPointUTF8 = StringCast<UTF8CHAR>(*EntryPoint);
			ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile(
				Options,
				{ PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 }, 
				(const char*)EntryPointUTF8.Get(),
				ShaderConductor::ShaderStage::PixelShader, TargetDesc);
			FString PatchedSource = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };

			if (!ShaderResultDesc.hasError)
			{
				FString Pattern = "_AppendExprDummy_()";
				int32 ReplaceIndex = PatchedSource.Find(Pattern, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				if(ReplaceIndex != INDEX_NONE)
				{
					PatchedSource.RemoveAt(ReplaceIndex, Pattern.Len());
					PatchedSource.InsertAt(ReplaceIndex, "_AppendExpr_(" + InExpression + ")");
					int32 RemoveStartIndex = PatchedSource.Find("void _AppendExprDummy_()", ESearchCase::IgnoreCase, ESearchDir::FromEnd, ReplaceIndex);
					int32 RemoveEndIndex = PatchedSource.Find("}", ESearchCase::IgnoreCase, ESearchDir::FromStart, RemoveStartIndex);
					PatchedSource.RemoveAt(RemoveStartIndex, RemoveEndIndex - RemoveStartIndex + 1);
					
					FString FileName = DebugShader->GetShaderName() + TEXT("ExprPatched") + FileExtension;
					FFileHelper::SaveStringToFile(PatchedSource, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / FileName));
					
					TRefCountPtr<GpuShader> PatchedShader = GGpuRhi->CreateShaderFromSource({
						.Source = MoveTemp(PatchedSource),
						.Type = DebugShader->GetShaderType(),
						.EntryPoint = EntryPoint,
						.Language = Lang
					});
					if (GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo, ExtraArgs))
					{
						auto DummyRenderTarget = GGpuRhi->CreateTexture({
							.Width = (uint32)PsInvocation.ViewPortDesc.Width,
							.Height = (uint32)PsInvocation.ViewPortDesc.Height,
							.Format = PsInvocation.PipelineDesc.Targets[0].TargetFormat,
							.Usage = GpuTextureUsage::RenderTarget
						});
						GpuRenderPipelineStateDesc PatchedPipelineDesc{
							.Vs = PsInvocation.PipelineDesc.Vs,
							.Ps = PatchedShader,
							.Targets = {{DummyRenderTarget->GetFormat()}},
							.RasterizerState = PsInvocation.PipelineDesc.RasterizerState,
							.Primitive = PsInvocation.PipelineDesc.Primitive
						};
						PatchedBindings.ApplyBindGroupLayout(PatchedPipelineDesc);
						TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PatchedPipelineDesc);
						
						GpuRenderPassDesc DummyPassDesc;
						DummyPassDesc.ColorRenderTargets.Add({ DummyRenderTarget->GetDefaultView() });
						
						auto CmdRecorder = GGpuRhi->BeginRecording();
						GpuResourceHelper::ClearRWResource(CmdRecorder, DebugBuffer);
						RenderGraph RG(CmdRecorder);
						RG.AddRenderPass(TEXT("ExprDebugger"), MoveTemp(DummyPassDesc), PatchedBindings,
							[&](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
								PassRecorder->SetViewPort(PsInvocation.ViewPortDesc);
								PassRecorder->SetRenderPipelineState(Pipeline);
								Bindings.ApplyBindGroup(PassRecorder);
								PsInvocation.DrawFunction(PassRecorder);
							}
						);
						RG.Execute();
						
						uint8* DebugBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebugBuffer, GpuResourceMapMode::Read_Only);
						SpvId TypeDescId = *(uint32*)(DebugBufferData);
						int32 ResultSize = *(int32*)(DebugBufferData + 16);
						TArray<uint8> ResultValue = { DebugBufferData + 32, ResultSize };
						GGpuRhi->UnMapGpuBuffer(DebugBuffer);
						
						SpvTypeDesc* ResultTypeDesc = ExprContext.TypeDescs[TypeDescId].Get();
						if (ResultTypeDesc && !ResultValue.IsEmpty())
						{
							FText TypeName = FText::FromString(GetTypeDescStr(ResultTypeDesc, Lang));
							
							TArray<Vector2i> ResultRange;
							ResultRange.Add({ 0, ResultValue.Num() });
							FString ValueStr = GetValueStr(ResultValue, ResultTypeDesc, ResultRange, 0, DebuggerViewHex);
							
							TArray<TSharedPtr<ExpressionNode>> Children;
							if (ResultTypeDesc->GetKind() == SpvTypeDescKind::Composite || ResultTypeDesc->GetKind() == SpvTypeDescKind::Array
								|| ResultTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
							{
								Children = AppendChildNodes(Lang, ResultTypeDesc, ResultRange, {}, ResultValue, 0);
							}
							
							return { .Expr = InExpression, .ValueStr = ValueStr,
								.TypeName = TypeName.ToString(), .Children = MoveTemp(Children) };
						}
						
					}
				}
			}
	
		}

		return { .Expr = InExpression, .ValueStr = LOCALIZATION("InvalidExpr").ToString() };
	}

	void ShaderDebugger::ShowDebuggerResult()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerCallStackView* DebuggerCallStackView = ShEditor->GetDebuggerCallStackView();
		TArray<CallStackDataPtr> CallStackDatas;

		GpuShaderLanguage Lang = DebugShader->GetShaderLanguage();
		SpvFunctionDesc* FuncDesc = GetFunctionDesc(Scope);
		FString Location = FString::Printf(TEXT("%s (Line %d)"), *StopLocation.File, StopLocation.LineNumber);
		CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc, Lang), MoveTemp(Location)));
		for (int i = CallStack.Num() - 1; i >= 0; i--)
		{
			SpvFunctionDesc* FuncDesc = GetFunctionDesc(CallStack[i].Scope);
			int32 ExtraLineNum = CurShaderAsset->FindIncludeAsset(CallStack[i].File)->GetExtraLineNum();
			int JumpLineNumber = CallStack[i].Line - ExtraLineNum;
			if (JumpLineNumber > 0)
			{
				FString Location = FString::Printf(TEXT("%s (Line %d)"), *CallStack[i].File, JumpLineNumber);
				CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc, Lang), MoveTemp(Location)));
			}
		}
		DebuggerCallStackView->SetCallStackDatas(CallStackDatas, DebuggerError);
		DebuggerCallStackView->ActiveData = CallStackDatas[0];
		ActiveCallPoint.reset();

		ShowDebuggerVariable(Scope);

		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();
		DebuggerWatchView->OnWatch = [this](const FString& Expression) { return EvaluateExpression(Expression); };
		DebuggerWatchView->Refresh();

		//Compute and render per-line preview thumbnails
		if (EnableLinePreview())
		{
			ComputeLinePreviewData();
			InvokePixelPreview();

			if (ShEditor->IsShowingLinePreview())
			{
				const DebuggerLocation& PreviewLoc = ShEditor->GetLinePreviewLocation();
				if (DirtyLinePreviewLocs.Contains(PreviewLoc))
				{
					ShEditor->ShowLinePreview(PreviewLoc);
				}
			}

			//Refresh LineTipList to show preview thumbnails
			for (auto ShaderEditor : ShEditor->GetShaderEditors())
			{
				ShaderEditor->RefreshLineTips();
			}
		}
	}

	void ShaderDebugger::ShowDebuggerVariable(SpvLexicalScope* InScope) const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		
		AssetPtr<ShaderAsset> StopShader = CurShaderAsset->FindIncludeAsset(StopLocation.File);
		int32 ExtraLineNum = StopShader->GetExtraLineNum();

		TArray<ExpressionNodePtr> LocalVarNodeDatas;
		TArray<ExpressionNodePtr> GlobalVarNodeDatas;
		GpuShaderLanguage Lang = DebugShader->GetShaderLanguage();
		if (!ReturnValue.IsEmpty())
		{
			SpvTypeDesc* ReturnTypeDesc = std::get<SpvTypeDesc*>(GetFunctionDesc(Scope)->GetFuncTypeDesc()->GetReturnType());
			FString VarName = GetFunctionDesc(Scope)->GetName() + LOCALIZATION("ReturnTip").ToString();
			FString TypeName = GetTypeDescStr(ReturnTypeDesc, Lang);
			FString ValueStr = GetValueStr(ReturnValue, ReturnTypeDesc, TArray{ Vector2i{0, ReturnValue.Num()} }, 0, DebuggerViewHex);

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
					bool VisibleLine = VarDesc->Line <= StopLocation.LineNumber + ExtraLineNum && VarDesc->Line > ExtraLineNum;
					if (VisibleScope && VisibleLine)
					{
						FString VarName = VarDesc->Name;
						if(VarName.StartsWith("GPrivate_"))
						{
							continue;
						}
						
						//If there are variables with the same name, only the one in the most recent scope is shown.
						if (LocalVarNodeDatas.ContainsByPredicate([&](const ExpressionNodePtr& InItem) { return InItem->Expr.Equals(VarName); }))
						{
							continue;
						}
						FString TypeName = GetTypeDescStr(VarDesc->TypeDesc, DebugShader->GetShaderLanguage());
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
						FString ValueStr = GetValueStr(Value, VarDesc->TypeDesc, InitializedRanges, 0, DebuggerViewHex);
						TArray<SpvVarDirtyRange> Ranges;
						DirtyVars.MultiFind(VarId, Ranges);
						auto Data = MakeShared<ExpressionNode>(VarName, ValueStr, TypeName, !Ranges.IsEmpty(), true);
						if (VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Composite || VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Array
							|| VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
						{
							Data->Children = AppendChildNodes(Lang, VarDesc->TypeDesc, InitializedRanges, Ranges, Value, 0);
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
		if (DebuggerError.Key == "Assert failed")
		{
			CurDebugStateIndex++;
		}
		DebuggerError = {};
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
		while (CurDebugStateIndex < DebugStates.Num())
		{
			const SpvDebugState& DebugState = DebugStates[CurDebugStateIndex];
			std::optional<int32> NextValidLine;
			SpvId NextSource{};
			if (CurDebugStateIndex + 1 < DebugStates.Num())
			{
				const SpvDebugState& NextDebugState = DebugStates[CurDebugStateIndex + 1];
				if (std::holds_alternative<SpvDebugState_VarChange>(NextDebugState))
				{
					const auto& State = std::get<SpvDebugState_VarChange>(NextDebugState);
					NextValidLine = State.Line;
					NextSource = State.Source;
				}
				else if (std::holds_alternative<SpvDebugState_FuncCall>(NextDebugState))
				{
					const auto& State = std::get<SpvDebugState_FuncCall>(NextDebugState);
					NextValidLine = State.Line;
					NextSource = State.Source;
				}
				else if (std::holds_alternative<SpvDebugState_Tag>(NextDebugState))
				{
					const auto& State = std::get<SpvDebugState_Tag>(NextDebugState);
					NextValidLine = State.Line;
					NextSource = State.Source;
				}
			}

			FString Error;
			ApplyDebugState(DebugState, Error);
			if (!Error.IsEmpty())
			{
				CurValidLine = StopLocation.LineNumber + GetExtraLineNumForFile(StopLocation.File);
				DebuggerError = { MoveTemp(Error), StopLocation };
				break;
			}
			CurDebugStateIndex++;

			if (NextValidLine && NextValidLine.value())
			{
				int32 NextSourceExtraLineNum = GetExtraLineNumForSource(NextSource);
				FString NextSourceFile = DebuggerContext->GetSourceFileName(NextSource);
				
				bool MatchBreakPoint = false;
				if (Scope)
				{
					TArray<int32> SourceBreakPoints;
					auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
					
					if (AssetPtr<ShaderAsset> IncludeShader = CurShaderAsset->FindIncludeAsset(NextSourceFile))
					{
						if (SShaderEditorBox* IncludeEditor = ShEditor->GetShaderEditor(IncludeShader))
						{
							SourceBreakPoints = IncludeEditor->BreakPointLineNumbers;
						}
					}
					
					MatchBreakPoint = SourceBreakPoints.ContainsByPredicate([&](int32 InEntry) {
						int32 BreakPointLine = InEntry + NextSourceExtraLineNum;
						int32 BreakPointValidLine = ValidLines[Algo::UpperBound(ValidLines, BreakPointLine - 1)];
						for (const auto& [_, BB] : DebuggerContext->BBs)
						{
							if (BB.ValidLines.Contains(BreakPointValidLine) && BB.ValidLines.Contains(NextValidLine))
							{
								if (!CurValidLine)
								{
									if (!GetFunctionDesc(Scope))
									{
										continue;
									}
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
				}
				bool SameStack = CurValidLine != NextValidLine && CallStackAtStop.Num() == CallStack.Num();
				bool ReturnStack = CallStackAtStop.Num() > CallStack.Num();
				bool CrossStack = CallStackAtStop.Num() != CallStack.Num();

				bool StopStepOver = Mode == StepMode::StepOver && NextValidLine.value() - NextSourceExtraLineNum > 0 && (SameStack || ReturnStack);
				bool StopStepInto = Mode == StepMode::StepInto && NextValidLine.value() - NextSourceExtraLineNum > 0 && (SameStack || CrossStack);

				CurValidLine = NextValidLine;
				if (MatchBreakPoint || StopStepOver || StopStepInto)
				{
					CallStackAtStop = CallStack;
					StopLocation.File = DebuggerContext->GetSourceFileName(NextSource);
					StopLocation.LineNumber = NextValidLine.value() - NextSourceExtraLineNum;
					break;
				}
			}
		}

		return CurDebugStateIndex < DebugStates.Num() && StopLocation.LineNumber > 0;
	}

	ShaderDebugger::ShaderDebugger()
	{
	}

	bool ShaderDebugger::EnableUbsan()
	{
		bool EnableUbsan = false;
		Editor::GetEditorConfig()->GetBool(TEXT("Environment"), TEXT("EnableUbsan"), EnableUbsan);
		return EnableUbsan;
	}

	bool ShaderDebugger::EnableLinePreview()
	{
		bool bEnable = true;
		Editor::GetEditorConfig()->GetBool(TEXT("Environment"), TEXT("EnableLinePreview"), bEnable);
		return bEnable;
	}

	int32 ShaderDebugger::GetExtraLineNumForSource(SpvId Source) const
	{
		if (!DebuggerContext || !CurShaderAsset)
		{
			return 0;
		}
		FString SourceFileName = DebuggerContext->GetSourceFileName(Source);
		return GetExtraLineNumForFile(SourceFileName);
	}

	int32 ShaderDebugger::GetExtraLineNumForFile(const FString& FileName) const
	{
		if (!DebuggerContext || !CurShaderAsset)
		{
			return 0;
		}
		if (AssetPtr<ShaderAsset> SourceShader = CurShaderAsset->FindIncludeAsset(FileName))
		{
			return SourceShader->GetExtraLineNum();
		}
		return CurShaderAsset->GetExtraLineNum();
	}

	void ShaderDebugger::ApplyDebugState(const SpvDebugState& InState, FString& Error)
	{
		if (std::holds_alternative<SpvDebugState_VarChange>(InState))
		{
			const auto& State = std::get<SpvDebugState_VarChange>(InState);
			if (!State.Error.IsEmpty())
			{
				if (bEnableUbsan)
				{
					Error = "Undefined";
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
					SH_LOG(LogShader, Error, TEXT("%s"), *State.Error);
				}
				return;
			}

			SpvVariable* Var = DebuggerContext->FindVar(State.Change.VarId);

			void* Dest = &Var->GetBuffer()[State.Change.ByteOffset];
			const void* Src = State.Change.NewDirtyValue.GetData();
			FMemory::Memcpy(Dest, Src, State.Change.NewDirtyValue.Num());

			SpvVarDirtyRange DirtyRange = { State.Change.ByteOffset, State.Change.NewDirtyValue.Num() };
			Var->InitializedRanges.Add({ State.Change.ByteOffset, State.Change.ByteOffset + State.Change.NewDirtyValue.Num() });
			DirtyVars.Add(State.Change.VarId, MoveTemp(DirtyRange));

			//Propagate changes made to the parameter back to the argument.
			//Value-type params have SSA value arguments which are not variables, skip them.
			if (DebuggerContext->IsParameter(Var->Id) && !CallStack.IsEmpty())
			{
				const SpvFuncCall* SourceCall = CallStack.Last().Call;
				const TArray<SpvId>& Parameters = DebuggerContext->Funcs.at(SourceCall->Callee).Parameters;
				for (int i = 0; i < SourceCall->Arguments.Num(); i++)
				{
					if (Parameters[i] == Var->Id)
					{
						SpvVariable* ArgumentVar = DebuggerContext->FindVar(SourceCall->Arguments[i]);
						if (!ArgumentVar)
							break;
						ArgumentVar->Storage = Var->Storage;
						ArgumentVar->InitializedRanges = Var->InitializedRanges;
						break;
					}
				}
			}
			
			if (AssertResult && AssertResult->IsCompletelyInitialized())
			{
				TArray<uint8>& Value = AssertResult->GetBuffer();
				uint32& TypedValue = *(uint32*)Value.GetData();
				if (TypedValue != 1)
				{
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
					Error = "Assert failed";
					TypedValue = 1;
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
				//Value-type params (non-pointer) are not tracked as variables, so skip them.
				if (!ArgumentVar || !ParameterVar)
					continue;
				ParameterVar->Storage = ArgumentVar->Storage;
				ParameterVar->InitializedRanges = ArgumentVar->InitializedRanges;
			}
			if (Scope)
			{
				FString SourceFile = DebuggerContext->GetSourceFileName(State.Source);
				CallStack.Emplace(Scope, State.Line, MoveTemp(SourceFile), CurDebugStateIndex, &Call);
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
			if (Var && Var->GetBufferSize() > 0)
			{
				auto AccessOrError = GetAccess(Var, State.Indexes);
				if (AccessOrError.HasError())
				{
					Error = "Undefined";
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
					SH_LOG(LogShader, Error, TEXT("%s"), *AccessOrError.GetError());
				}
				else
				{
					bool Uninitialized = false;
					auto [Type, ByteOffset] = AccessOrError.GetValue();
					int32 Size = GetTypeByteSize(Type);
					if (State.CheckMask != 0xFFFFFFFFu)
					{
						// Only check init-units whose bits are set in CheckMask (each init-unit = 4 bytes).
						// CheckMask is relative to the OpLoad result type, so offset by ByteOffset.
						for (uint32 Bit = 0; Bit < (uint32)(Size / 4); Bit++)
						{
							if (!(State.CheckMask & (1u << Bit)))
								continue;
							int32 UnitStart = ByteOffset + Bit * 4;
							int32 UnitEnd = UnitStart + 4;
							if (UnitEnd > Var->GetBufferSize())
								break;
							bool UnitInit = false;
							for (const Vector2i& R : Var->InitializedRanges)
							{
								if (R.X <= UnitStart && R.Y >= UnitEnd)
								{
									UnitInit = true;
									break;
								}
							}
							if (!UnitInit)
							{
								Uninitialized = true;
								break;
							}
						}
					}
					else
					{
						Uninitialized = !Var->IsRangeInitialized(ByteOffset, ByteOffset + Size);
					}
					if (Uninitialized)
					{
						Error = "Undefined";
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
				StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
				StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
						SH_LOG(LogShader, Error, TEXT("pow: x(%f) < 0 for component %d, the result is undefined."), X, i);
					}
					else if (X == 0.0f && Y <= 0.0f)
					{
						Error = "Undefined";
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
					SH_LOG(LogShader, Error, TEXT("pow: x(%f) < 0, the result is undefined."), X);
				}
				else if (X == 0.0f && Y <= 0.0f)
				{
					Error = "Undefined";
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
								StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
								StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
								StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
								StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
							SH_LOG(LogShader, Error, TEXT("Division by zero is undefined for component %d."), i);
						}
					}
					else if (static_cast<SpvIntegerType*>(ResultType)->IsSigend())
					{
						int Operand2 = *(int*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
							SH_LOG(LogShader, Error, TEXT("Division by zero is undefined for component %d."), i);
						}
					}
					else
					{
						uint32 Operand2 = *(uint32*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
							SH_LOG(LogShader, Error, TEXT("Conversion(float to int) is undefined for component %d: It's not wide enough to hold the converted value(%f)."), i, f);
						}
					}
					else
					{
						if (f < 0.0f || uint64(f) > uint64(std::numeric_limits<uint32>::max()))
						{
							Error = "Undefined";
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
						SH_LOG(LogShader, Error, TEXT("Conversion(float to int) is undefined: It's not wide enough to hold the converted value(%f)."), f);
					}
				}
				else
				{
					if (f < 0.0f || uint64(f) > uint64(std::numeric_limits<uint32>::max()))
					{
						Error = "Undefined";
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
							SH_LOG(LogShader, Error, TEXT("int remainder by zero is undefined for component %d."), i);
						}
						else if (Operand2 == -1 && Operand1 == std::numeric_limits<int32>::min())
						{
							Error = "Undefined";
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
							SH_LOG(LogShader, Error, TEXT("The remainder causing signed overflow is undefined for component %d."), i);
						}
					}
					else
					{
						uint32 Operand2 = *(uint32*)(State.Operand2.GetData() + i * 4);
						if (Operand2 == 0)
						{
							Error = "Undefined";
							StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
							StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
						SH_LOG(LogShader, Error, TEXT("int remainder by zero is undefined."));
					}
					else if (Operand2 == -1 && Operand1 == std::numeric_limits<int32>::min())
					{
						Error = "Undefined";
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
						SH_LOG(LogShader, Error, TEXT("The remainder causing signed overflow is undefined."));
					}
				}
				else
				{
					uint32 Operand2 = *(uint32*)(State.Operand2.GetData());
					if (Operand2 == 0)
					{
						Error = "Undefined";
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
						StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
						StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
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
					StopLocation.File = DebuggerContext->GetSourceFileName(State.Source);
					StopLocation.LineNumber = State.Line - GetExtraLineNumForSource(State.Source);
					SH_LOG(LogShader, Error, TEXT("Atan2: y = x = 0, the result is undefined."));
				}
			}
		}
	}

	void ShaderDebugger::InvokePixel(bool GlobalValidation)
	{
		const auto& PsInvocation = std::get<PixelState>(Invocation);

		DebugShader = GGpuRhi->CreateShaderFromSource(CurShaderAsset->GetShaderDesc(ShaderSourceText));
		DebugShader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;
		GpuShaderLanguage Lang = DebugShader->GetShaderLanguage();

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		ExtraArgs.Add("-ignore-validation-error");

		FString ErrorInfo, WarnInfo;
		if (!GGpuRhi->CompileShader(DebugShader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			throw std::runtime_error(TCHAR_TO_UTF8(*ErrorInfo));
		}

		uint32 BufferSize = GlobalValidation ? 1024 : 1024 * 1024 * 4;
		TArray<uint8> Datas;
		Datas.SetNumZeroed(BufferSize);
		DebugBuffer = GGpuRhi->CreateBuffer({
			.ByteSize = BufferSize,
			.Usage = GpuBufferUsage::RWRaw,
			.InitialData = Datas,
		});

		// Create DebugParamsBuffer with PixelCoord (uvec2)
		TArray<uint8> ParamsDatas;
		ParamsDatas.SetNumZeroed(sizeof(Vector2u));
		FMemory::Memcpy(ParamsDatas.GetData(), &PixelCoord, sizeof(Vector2u));
		DebugParamsBuffer = GGpuRhi->CreateBuffer({
			.ByteSize = sizeof(Vector2u),
			.Usage = GpuBufferUsage::Uniform,
			.InitialData = ParamsDatas,
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
						.Type = LayoutBindingEntry.Type,
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
					BindGroupDesc.Resources.Add(DebuggerBufferBindingSlot, { AUX::StaticCastRefCountPtr<GpuResource>(DebugBuffer) });
					LayoutDesc.Layouts.Add(DebuggerBufferBindingSlot, { BindingType::RWRawBuffer });

					//Add the debugger params buffer (contains PixelCoord)
					BindGroupDesc.Resources.Add(DebuggerParamsBindingSlot, { AUX::StaticCastRefCountPtr<GpuResource>(DebugParamsBuffer) });
					LayoutDesc.Layouts.Add(DebuggerParamsBindingSlot, { BindingType::UniformBuffer });

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

		FString FileExtension = (Lang == GpuShaderLanguage::HLSL) ? TEXT(".hlsl") : TEXT(".glsl");
		FString EntryPoint = DebugShader->GetEntryPoint();

		SpirvParser Parser;
		Parser.Parse(DebugShader->SpvCode);
		SpvMetaVisitor MetaVisitor{ *PixelDebuggerContext };
		Parser.Accept(&MetaVisitor);
		TArray<uint32> PatchedSpv;
		FString PatchedSpvAsm;
		if (GlobalValidation)
		{
			SpvValidator Validator{ *PixelDebuggerContext, bEnableUbsan, ShaderType::PixelShader };
			Parser.Accept(&Validator);
			PatchedSpv = Validator.GetPatcher().GetSpv();
			PatchedSpvAsm = Validator.GetPatcher().GetAsm();
			FFileHelper::SaveStringToFile(PatchedSpvAsm, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Validation" + FileExtension + ".spvasm"));
		}
		else
		{
			SpvPixelDebuggerVisitor DebuggerVisitor{ *PixelDebuggerContext, Lang, bEnableUbsan };
			Parser.Accept(&DebuggerVisitor);
			PatchedSpv = DebuggerVisitor.GetPatcher().GetSpv();
			PatchedSpvAsm = DebuggerVisitor.GetPatcher().GetAsm();
			FFileHelper::SaveStringToFile(PatchedSpvAsm, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Patched" + FileExtension + ".spvasm"));
		}
		
		CurDebugStateIndex = 0;
		DebuggerContext = MoveTemp(PixelDebuggerContext);
		TRefCountPtr<GpuShader> PatchedShader = GGpuRhi->CreateShaderFromSource({
			.Name = DebugShader->GetShaderName(),
			.Source = PatchedSpvAsm, //for pso cache
			.Type = DebugShader->GetShaderType(),
			.EntryPoint = EntryPoint,
		});
		PatchedShader->SpvCode = MoveTemp(PatchedSpv);
		PatchedShader->CompilerFlag |= GpuShaderCompilerFlag::CompileFromSpvCode;
		if (!GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			throw std::runtime_error(TCHAR_TO_UTF8(*ErrorInfo));
		}
		 
		auto DummyRenderTarget = GGpuRhi->CreateTexture({
			.Width = (uint32)PsInvocation.ViewPortDesc.Width,
			.Height = (uint32)PsInvocation.ViewPortDesc.Height,
			.Format = PsInvocation.PipelineDesc.Targets[0].TargetFormat,
			.Usage = GpuTextureUsage::RenderTarget
		});
		GpuRenderPipelineStateDesc PatchedPipelineDesc{
			.Vs = PsInvocation.PipelineDesc.Vs,
			.Ps = PatchedShader,
			.Targets = {{DummyRenderTarget->GetFormat()}},
			.RasterizerState = PsInvocation.PipelineDesc.RasterizerState,
			.Primitive = PsInvocation.PipelineDesc.Primitive
		};
		PatchedBindings.ApplyBindGroupLayout(PatchedPipelineDesc);
		TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PatchedPipelineDesc);

		GpuRenderPassDesc DummyPassDesc;
		DummyPassDesc.ColorRenderTargets.Add({ DummyRenderTarget->GetDefaultView() });

		auto CmdRecorder = GGpuRhi->BeginRecording();
		RenderGraph RG(CmdRecorder);
		RG.AddRenderPass(TEXT("Debugger"), MoveTemp(DummyPassDesc), PatchedBindings,
			[&](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
				PassRecorder->SetViewPort(PsInvocation.ViewPortDesc);
				PassRecorder->SetRenderPipelineState(Pipeline);
				Bindings.ApplyBindGroup(PassRecorder);
				PsInvocation.DrawFunction(PassRecorder);
			}
		);
		RG.Execute();
	}

	void ShaderDebugger::DebugPixel(const Vector2u& InPixelCoord, const InvocationState& InState)
	{
		PixelCoord = InPixelCoord;
		Invocation = InState;
		bEnableUbsan = EnableUbsan();
		
		InvokePixel();
		uint8* DebugBufferData = (uint8*)GGpuRhi->MapGpuBuffer(DebugBuffer, GpuResourceMapMode::Read_Only);
		DebugStates = GenDebugStates(DebugBufferData);
		GGpuRhi->UnMapGpuBuffer(DebugBuffer);
		
#if !SH_SHIPPING
		DumpDebugStatesToFile(DebugStates, DebuggerContext.Get(), PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / TEXT("DebugStates.txt"));
#endif

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

	void ShaderDebugger::ComputeLinePreviewData()
	{
		if (!DebuggerContext || DebugStates.IsEmpty())
		{
			return;
		}

		//Only scan newly traversed states since last call
		int32 StartIdx = PrevPreviewStateIndex;

		DirtyLinePreviewLocs.Empty();

		SpvFunctionDesc* CurFuncDesc = GetFunctionDesc(Scope);
		SpvLexicalScope* ScanScope = PrevPreviewScope;
		uint32 CallStackHash = PrevPreviewCallStackHash;
		TArray<SpvId> CallIdStack = PrevPreviewCallIdStack;

		auto IsInCurrentFunc = [&]() { return ScanScope && GetFunctionDesc(ScanScope) == CurFuncDesc; };

		TSet<DebuggerLocation> ConditionLines;
		for (int32 i = StartIdx; i < CurDebugStateIndex && i < DebugStates.Num(); i++)
		{
			if (std::holds_alternative<SpvDebugState_ScopeChange>(DebugStates[i]))
			{
				ScanScope = std::get<SpvDebugState_ScopeChange>(DebugStates[i]).Change.NewScope;
				continue;
			}

			if (std::holds_alternative<SpvDebugState_Tag>(DebugStates[i]))
			{
				const auto& Tag = std::get<SpvDebugState_Tag>(DebugStates[i]);
				if (Tag.bCondition)
				{
					FString TagFile = DebuggerContext->GetSourceFileName(Tag.Source);
					int32 TagLine = Tag.Line - GetExtraLineNumForSource(Tag.Source);
					if (TagLine > 0)
					{
						ConditionLines.Add({ MoveTemp(TagFile), TagLine });
					}
				}
				if (Tag.bReturn && !CallIdStack.IsEmpty())
				{
					SpvId CallId = CallIdStack.Pop();
					CallStackHash = (CallStackHash - CallId.GetValue()) * 3186588639u;
				}
				continue;
			}

			if (std::holds_alternative<SpvDebugState_FuncCall>(DebugStates[i]))
			{
				const auto& FC = std::get<SpvDebugState_FuncCall>(DebugStates[i]);
				CallIdStack.Push(FC.CallId);
				CallStackHash = CallStackHash * 31 + FC.CallId.GetValue();
				continue;
			}

			if (!std::holds_alternative<SpvDebugState_VarChange>(DebugStates[i]))
			{
				continue;
			}

			const auto& VarChange = std::get<SpvDebugState_VarChange>(DebugStates[i]);
			if (VarChange.Error.IsEmpty())
			{
				//Skip non-user-declared variables
				FString VarName;
				auto It = DebuggerContext->VariableDescMap.find(VarChange.Change.VarId);
				if (It == DebuggerContext->VariableDescMap.end())
				{
					continue;
				}

				VarName = It->second->Name;
				if (VarName.StartsWith("GPrivate_"))
				{
					continue;
				}

				//Only support scalar and vector types for preview
				SpvTypeDescKind TypeKind = It->second->TypeDesc->GetKind();
				if (TypeKind != SpvTypeDescKind::Basic && TypeKind != SpvTypeDescKind::Vector)
				{
					continue;
				}

				//Skip constant-value stores — useless for preview
				uint32 PH = FW::PackDebugHeader(FW::SpvDebuggerStateType::VarChange, VarChange.Source.GetValue(), (uint32)VarChange.Line);
				uint64 ConstKey = ((uint64)PH << 32) | VarChange.Change.VarId.GetValue();
				if (DebuggerContext->ConstInitVarChanges.Contains(ConstKey))
				{
					continue;
				}

				int32 LineNumber = VarChange.Line - GetExtraLineNumForSource(VarChange.Source);
				FString SourceFile = DebuggerContext->GetSourceFileName(VarChange.Source);
				if (LineNumber > 0)
				{
					DebuggerLocation Loc{ SourceFile, LineNumber };
					LinePreviewInfo& Existing = LinePreviewData.FindOrAdd(Loc);
					int32 Iteration = (Existing.VarId == VarChange.Change.VarId && Existing.CallStackHash == CallStackHash) ? Existing.IterationIndex + 1 : 0;
					Existing = { VarChange.Change.VarId, Iteration, PH, MoveTemp(VarName), CallStackHash };
					if (IsInCurrentFunc())
					{
						DirtyLinePreviewLocs.Add(Loc);
					}
				}
			}
		}

		//Remove lines that are also condition lines
		for (const DebuggerLocation& CondLoc : ConditionLines)
		{
			LinePreviewData.Remove(CondLoc);
			DirtyLinePreviewLocs.Remove(CondLoc);
		}

		PrevPreviewStateIndex = CurDebugStateIndex;
		PrevPreviewScope = Scope;
		PrevPreviewCallStackHash = CallStackHash;
		PrevPreviewCallIdStack = MoveTemp(CallIdStack);
	}

	void ShaderDebugger::InvokePixelPreview()
	{
		if (DirtyLinePreviewLocs.IsEmpty())
		{
			return;
		}

		const auto& PsInvocation = std::get<PixelState>(Invocation);
		GpuShaderLanguage Lang = DebugShader->GetShaderLanguage();
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		ExtraArgs.Add("-ignore-validation-error");

		SpvPixelPreviewerContext PreviewContext{ SpvBindings };
		SpirvParser Parser;
		Parser.Parse(DebugShader->SpvCode);
		SpvMetaVisitor MetaVisitor{ PreviewContext };
		Parser.Accept(&MetaVisitor);
		SpvPixelPreviewerVisitor PreviewVisitor{ PreviewContext, Lang };
		Parser.Accept(&PreviewVisitor);
		PreviewVisitor.GetPatcher().Dump(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / DebugShader->GetShaderName() + "Preview.spvasm");

		const TArray<uint32>& PatchedSpv = PreviewVisitor.GetPatcher().GetSpv();

		ShaderConductor::Compiler::TargetDesc TargetDesc{};
		ShaderConductor::Compiler::Options Options{};
		Options.force_zero_initialized_variables = true;
		FString FileExtension;
		FString EntryPoint = DebugShader->GetEntryPoint();
		if (Lang == GpuShaderLanguage::HLSL)
		{
			TargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
			TargetDesc.version = "66";
			FileExtension = TEXT(".hlsl");
		}
		else
		{
			Options.vulkanSemantics = true;
			TargetDesc.language = ShaderConductor::ShadingLanguage::Glsl;
			TargetDesc.version = "450";
			FileExtension = TEXT(".glsl");
		}

		auto EntryPointUTF8 = StringCast<UTF8CHAR>(*EntryPoint);
		ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile(
			Options, { PatchedSpv.GetData(), (uint32)PatchedSpv.Num() * 4 },
			(const char*)EntryPointUTF8.Get(),
			ShaderConductor::ShaderStage::PixelShader, TargetDesc);

		if (ShaderResultDesc.hasError)
		{
			SH_LOG(LogDebugger, Error, TEXT("[Preview] SpvCross error: %s"),
				*FString(static_cast<const char*>(ShaderResultDesc.errorWarningMsg.Data())));
			return;
		}

		FString PatchedSource = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };
		FFileHelper::SaveStringToFile(PatchedSource, *(PathHelper::SavedShaderDir() / DebugShader->GetShaderName() / (DebugShader->GetShaderName() + TEXT("Preview") + FileExtension)));

		FString ErrorInfo, WarnInfo;
		TRefCountPtr<GpuShader> PatchedShader = GGpuRhi->CreateShaderFromSource({
			.Name = DebugShader->GetShaderName(),
			.Source = PatchedSource,
			.Type = DebugShader->GetShaderType(),
			.EntryPoint = EntryPoint,
			.Language = Lang,
		});
		if (!GGpuRhi->CompileShader(PatchedShader, ErrorInfo, WarnInfo, ExtraArgs))
		{
			SH_LOG(LogDebugger, Error, TEXT("[Preview] Compile error: %s"), *ErrorInfo);
			return;
		}

		//Setup bindings: rebuild from SpvBindings, grouped by DescriptorSet
		TMap<int32, GpuBindGroupDesc> BaseGroupDescs;
		TMap<int32, GpuBindGroupLayoutDesc> BaseLayoutDescs;
		for (const auto& Binding : SpvBindings)
		{
			BaseGroupDescs.FindOrAdd(Binding.DescriptorSet).Resources.Add(Binding.Binding, { Binding.Resource });
			auto& LayoutDesc = BaseLayoutDescs.FindOrAdd(Binding.DescriptorSet);
			LayoutDesc.GroupNumber = Binding.DescriptorSet;
			LayoutDesc.Layouts.Add(Binding.Binding, { Binding.Type });
		}

		//Add previewer params slot to layout
		BaseLayoutDescs.FindOrAdd(BindingContext::GlobalSlot).Layouts.Add(PreviewerParamsBindingSlot, { BindingType::UniformBuffer });

		//Add debugger buffer slot
		BaseLayoutDescs.FindOrAdd(BindingContext::GlobalSlot).Layouts.Add(DebuggerBufferBindingSlot, { BindingType::RWRawBuffer });
		BaseGroupDescs.FindOrAdd(BindingContext::GlobalSlot).Resources.Add(DebuggerBufferBindingSlot, { AUX::StaticCastRefCountPtr<GpuResource>(DebugBuffer) });

		//Build non-global bind groups (shared across all lines)
		TRefCountPtr<GpuBindGroupLayout> GlobalLayout;
		BindingContext SharedBindings;
		for (auto& [SetNumber, LayoutDesc] : BaseLayoutDescs)
		{
			TRefCountPtr<GpuBindGroupLayout> Layout = GGpuRhi->CreateBindGroupLayout(LayoutDesc);
			if (SetNumber == BindingContext::GlobalSlot)
			{
				GlobalLayout = Layout;
				SharedBindings.SetGlobalBindGroupLayout(Layout);
			}
			else
			{
				BaseGroupDescs[SetNumber].Layout = Layout;
				TRefCountPtr<GpuBindGroup> Group = GGpuRhi->CreateBindGroup(BaseGroupDescs[SetNumber]);
				if (SetNumber == BindingContext::PassSlot)
				{
					SharedBindings.SetPassBindGroup(Group);
					SharedBindings.SetPassBindGroupLayout(Layout);
				}
				else if (SetNumber == BindingContext::ShaderSlot)
				{
					SharedBindings.SetShaderBindGroup(Group);
					SharedBindings.SetShaderBindGroupLayout(Layout);
				}
			}
		}
		 
		GpuRenderPipelineStateDesc PreviewPipelineDesc{
			.Vs = PsInvocation.PipelineDesc.Vs,
			.Ps = PatchedShader,
			.Targets = {{ GpuFormat::R32G32B32A32_FLOAT }},
			.RasterizerState = PsInvocation.PipelineDesc.RasterizerState,
			.Primitive = PsInvocation.PipelineDesc.Primitive
		};
		SharedBindings.ApplyBindGroupLayout(PreviewPipelineDesc);
		TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PreviewPipelineDesc);

		//Per-line: create params buffer, bind group, render target, and collect render passes
		struct PerLineRenderData
		{
			TRefCountPtr<GpuBuffer> ParamsBuffer;
			TRefCountPtr<GpuTexture> RenderTarget;
			BindingContext Bindings;
		};
		TMap<DebuggerLocation, PerLineRenderData> PerLineData;

		//Generate CPU-side grid pattern for preview RT initialization
		uint32 TexWidth = (uint32)PsInvocation.ViewPortDesc.Width;
		uint32 TexHeight = (uint32)PsInvocation.ViewPortDesc.Height;
		TArray<uint8> GridData;
		GridData.SetNum(TexWidth * TexHeight * 4 * sizeof(float));
		float* GridPixels = reinterpret_cast<float*>(GridData.GetData());
		for (uint32 y = 0; y < TexHeight; y++)
		{
			for (uint32 x = 0; x < TexWidth; x++)
			{
				bool Odd = ((x / 8) + (y / 8)) % 2 != 0;
				float Val = Odd ? 0.15f : 0.1f;
				float* P = GridPixels + (y * TexWidth + x) * 4;
				P[0] = Val; P[1] = Val; P[2] = Val; P[3] = 1.0f;
			}
		}

		for (const DebuggerLocation& Loc : DirtyLinePreviewLocs)
		{
			const LinePreviewInfo* InfoPtr = LinePreviewData.Find(Loc);
			if (!InfoPtr || !InfoPtr->VarId.IsValid())
			{
				continue;
			}
			const LinePreviewInfo& Info = *InfoPtr;
			TArray<uint8> ParamsDatas;
			ParamsDatas.SetNumZeroed(4 * sizeof(uint32));
			uint32 TargetIteration = (uint32)Info.IterationIndex;
			uint32 TargetPackedHeader = Info.PackedHeader;
			uint32 TargetVarId = Info.VarId.GetValue();
			uint32 TargetCallStackHash = Info.CallStackHash;
			FMemory::Memcpy(ParamsDatas.GetData(), &TargetIteration, sizeof(uint32));
			FMemory::Memcpy(ParamsDatas.GetData() + sizeof(uint32), &TargetPackedHeader, sizeof(uint32));
			FMemory::Memcpy(ParamsDatas.GetData() + 2 * sizeof(uint32), &TargetVarId, sizeof(uint32));
			FMemory::Memcpy(ParamsDatas.GetData() + 3 * sizeof(uint32), &TargetCallStackHash, sizeof(uint32));

			TRefCountPtr<GpuBuffer> ParamsBuffer = GGpuRhi->CreateBuffer({
				.ByteSize = 4 * sizeof(uint32),
				.Usage = GpuBufferUsage::Uniform,
				.InitialData = ParamsDatas,
			});

			//Build global bind group with this line's params buffer
			GpuBindGroupDesc GlobalGroupDesc = BaseGroupDescs.FindOrAdd(BindingContext::GlobalSlot);
			GlobalGroupDesc.Resources.Add(PreviewerParamsBindingSlot, { AUX::StaticCastRefCountPtr<GpuResource>(ParamsBuffer) });
			GlobalGroupDesc.Layout = GlobalLayout;
			TRefCountPtr<GpuBindGroup> GlobalGroup = GGpuRhi->CreateBindGroup(GlobalGroupDesc);

			auto RenderTarget = GGpuRhi->CreateTexture({
				.Width = TexWidth,
				.Height = TexHeight,
				.Format = GpuFormat::R32G32B32A32_FLOAT,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
				.InitialData = GridData,
			});

			BindingContext LineBindings = SharedBindings;
			LineBindings.SetGlobalBindGroup(GlobalGroup);

			PerLineData.Add(Loc, { ParamsBuffer, RenderTarget, MoveTemp(LineBindings) });
		}

		RenderGraph RG;
		for (auto& [Loc, Data] : PerLineData)
		{
			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add({ .View = Data.RenderTarget->GetDefaultView(), .LoadAction = RenderTargetLoadAction::Load, .StoreAction = RenderTargetStoreAction::Store });

			RG.AddRenderPass(TEXT("Preview"), MoveTemp(PassDesc), Data.Bindings,
				[&](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					PassRecorder->SetViewPort(PsInvocation.ViewPortDesc);
					PassRecorder->SetRenderPipelineState(Pipeline);
					Bindings.ApplyBindGroup(PassRecorder);
					PsInvocation.DrawFunction(PassRecorder);
				}
			);

			LinePreviewTextures.Add(Loc, Data.RenderTarget);
		}
		RG.Execute();
	}

	void ShaderDebugger::Reset()
	{
		CurValidLine.reset();
		CallStack.Empty();
		DebuggerContext = nullptr;
		Scope = nullptr;
		ActiveCallPoint.reset();
		AssertResult = nullptr;
		DebuggerError = {};
		DirtyVars.Empty();
		StopLocation.Reset();
		ReturnValue.Empty();
		DebugStates.Reset();
		SortedVariableDescs.clear();
		SpvBindings.Empty();
		DebugShader = nullptr;
		DebugBuffer = nullptr;
		DebugParamsBuffer = nullptr;
		bEnableUbsan = false;
		bEditDuringDebugging = false;

		LinePreviewData.Empty();
		DirtyLinePreviewLocs.Empty();
		LinePreviewTextures.Empty();
		PrevPreviewStateIndex = 0;
		PrevPreviewScope = nullptr;
		PrevPreviewCallStackHash = 0;
		PrevPreviewCallIdStack.Empty();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();
		DebuggerLocalVariableView->SetVariableNodeDatas({});
		DebuggerGlobalVariableView->SetVariableNodeDatas({});
		DebuggerLocalVariableView->SetOnShowChanged(nullptr);
		DebuggerGlobalVariableView->SetOnShowChanged(nullptr);
	}

	void ShaderDebugger::InitDebuggerView()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->InvokeDebuggerTabs();
		SDebuggerCallStackView* DebuggerCallStackView = ShEditor->GetDebuggerCallStackView();
		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();

		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
		SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		DebuggerLocalVariableView->SetOnShowChanged([this] { 
			if (ActiveCallPoint)
			{
				ShowDebuggerVariable(ActiveCallPoint.value().Scope);
			}
			else
			{
				ShowDebuggerVariable(Scope);
			}
		});
		DebuggerGlobalVariableView->SetOnShowChanged([this] {
			if (ActiveCallPoint)
			{
				ShowDebuggerVariable(ActiveCallPoint.value().Scope);
			}
			else
			{
				ShowDebuggerVariable(Scope);
			}
		});

		DebuggerCallStackView->OnSelectionChanged = [this, DebuggerWatchView](const FString& FuncName) {
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			if (GetFunctionSig(GetFunctionDesc(Scope), DebugShader->GetShaderLanguage()) == FuncName)
			{
				StopLocation.File = DebuggerContext->GetSourceFileName(Scope->GetSource());
				int32 ExtraLineNum = CurShaderAsset->GetExtraLineNum();
				AssetPtr<ShaderAsset> StopShader = CurShaderAsset->FindIncludeAsset(StopLocation.File);
				ExtraLineNum = StopShader->GetExtraLineNum();
				AssetOp::OpenAsset(StopShader);
				if (SShaderEditorBox* StopEditor = ShEditor->GetShaderEditor(StopShader))
				{
					StopLocation.LineNumber = CurValidLine.value() - ExtraLineNum;
					StopEditor->JumpTo(StopEditor->GetLineIndex(StopLocation.LineNumber));
				}
				ShowDebuggerVariable(Scope);
				ActiveCallPoint.reset();
			}
			else if (auto CallPoint = CallStack.FindByPredicate([this, FuncName](const auto& InItem) {
				if (GetFunctionSig(GetFunctionDesc(InItem.Scope), DebugShader->GetShaderLanguage()) == FuncName)
				{
					return true;
				}
				return false;
			}))
			{
				int32 ExtraLineNum = CurShaderAsset->GetExtraLineNum();
				StopLocation.File = CallPoint->File;
				AssetPtr<ShaderAsset> StopShader = CurShaderAsset->FindIncludeAsset(CallPoint->File);
				ExtraLineNum = StopShader->GetExtraLineNum();
				AssetOp::OpenAsset(StopShader);
				if (SShaderEditorBox* EntryEditor = ShEditor->GetShaderEditor(StopShader))
				{
					StopLocation.LineNumber = CallPoint->Line - ExtraLineNum;
					EntryEditor->JumpTo(EntryEditor->GetLineIndex(StopLocation.LineNumber));
				}
				ShowDebuggerVariable(CallPoint->Scope);
				ActiveCallPoint = *CallPoint;
			}
			DebuggerWatchView->Refresh();
		};
	}

	TArray<SpvDebugState> ShaderDebugger::GenDebugStates(uint8* DebuggerData)
	{
		TArray<SpvDebugState> States;
		int Offset = 0;
		uint32 PackedHeader = *(uint32*)(DebuggerData + Offset);
		SpvDebuggerStateType StateType;
		uint32 UnpackedSource;
		uint32 UnpackedLine;
		UnpackDebugHeader(PackedHeader, StateType, UnpackedSource, UnpackedLine);
		SpvLexicalScope* PreScope = nullptr;

		// Call stack to track FuncCall Line/Source for inferring FuncCallAfterReturn
		struct FuncCallInfo { int32 Line; SpvId Source; };
		TArray<FuncCallInfo> GenCallStack;
		bool bJustEnteredCall = false;

		while (StateType != SpvDebuggerStateType::None)
		{
			if (SpvId* ScopeIdPtr = DebuggerContext->HeaderToScope.Find(PackedHeader))
			{
				SpvLexicalScope* NewScope = DebuggerContext->LexicalScopes[*ScopeIdPtr].Get();
				if (NewScope != PreScope)
				{
					// Detect function return: scope changes from callee function to caller function
					// bJustEnteredCall is true when the previous state was a FuncCall (entering a function, not returning)
					SpvFunctionDesc* PreFuncDesc = GetFunctionDesc(PreScope);
					SpvFunctionDesc* NewFuncDesc = GetFunctionDesc(NewScope);
					bool bFuncReturn = PreFuncDesc && NewFuncDesc && PreFuncDesc != NewFuncDesc
						&& !bJustEnteredCall && !GenCallStack.IsEmpty();

					States.Add(SpvDebugState_ScopeChange{
						{
							.PreScope = PreScope,
							.NewScope = NewScope,
						}
					});

					if (bFuncReturn)
					{
						auto CallInfo = GenCallStack.Pop();
						States.Add(SpvDebugState_Tag{
							.Line = CallInfo.Line,
							.Source = CallInfo.Source,
							.bFuncCallAfterReturn = true,
						});
					}

					PreScope = NewScope;
				}
			}

			bJustEnteredCall = false;
			Offset += sizeof(uint32);
			int32 Line = (int32)UnpackedLine;
			SpvId Source = SpvId(UnpackedSource);
			switch (StateType)
			{
			case SpvDebuggerStateType::VarChange:
			{
				SpvId VarId = *(SpvId*)(DebuggerData + Offset);
				Offset += 4;
				int32 IndexNum = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				TArray<int32> Indexes = { (int32*)(DebuggerData + Offset), IndexNum};
				Offset += IndexNum * 4;

				SpvVariable* DirtyVar = DebuggerContext->FindVar(VarId);
				auto AccessOrError = GetAccess(DirtyVar, Indexes);
				if (!AccessOrError.HasError())
				{
					const auto& [AccessedType, ByteOffset] = AccessOrError.GetValue();
					int32 ValueSize = GetTypeByteSize(AccessedType);
					TArray<uint8> PreDirtyValue = { &DirtyVar->GetBuffer()[ByteOffset], ValueSize };
					TArray<uint8> DirtyValue = {DebuggerData + Offset, ValueSize};
					Offset += ValueSize;

					auto NewDebugState = SpvDebugState_VarChange{
						.Line = Line,
						.Source = Source,
						.Change = {
							.VarId = VarId,
							.PreDirtyValue = MoveTemp(PreDirtyValue),
							.NewDirtyValue = MoveTemp(DirtyValue),
							.ByteOffset = ByteOffset,
						},
					};
					States.Add(MoveTemp(NewDebugState));
				}
				else
				{
					// Still need to advance past the value data in the buffer.
					Offset += GetTypeByteSize(GetAccessedType(DirtyVar, Indexes));

					auto NewDebugState = SpvDebugState_VarChange{
						.Line = Line,
						.Source = Source,
						.Error = AccessOrError.GetError()
					};
					States.Add(MoveTemp(NewDebugState));
				}

				break;
			}
			case SpvDebuggerStateType::ReturnValue:
			{
				int32 ValueSize = *(int32*)(DebuggerData + Offset);
				Offset += 4;
				TArray<uint8> ReturnValue = { DebuggerData + Offset, ValueSize };
				Offset += ReturnValue.Num();
				States.Add(SpvDebugState_ReturnValue{
					.Line = Line,
					.Source = Source,
					.Value = MoveTemp(ReturnValue)
				});
				break;
			}
			case SpvDebuggerStateType::Access:
			{
				SpvId VarId = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 IndexNum = *(int32*)(DebuggerData + Offset); Offset += 4;
				TArray<int32> Indexes = { (int32*)(DebuggerData + Offset), IndexNum }; Offset += IndexNum * 4;
				uint64 MaskKey = ((uint64)PackedHeader << 32) | VarId.GetValue();
				uint32 CheckMask = DebuggerContext->AccessCheckMasks.Contains(MaskKey) ? DebuggerContext->AccessCheckMasks[MaskKey] : 0xFFFFFFFFu;
				States.Add(SpvDebugState_Access{
					.Line = Line,
					.Source = Source,
					.VarId = VarId,
					.Indexes = MoveTemp(Indexes),
					.CheckMask = CheckMask
				});
				break;
			}
			case SpvDebuggerStateType::Normalize:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_Normalize{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::SmoothStep:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Edge0 = { DebuggerData + Offset, Size }; Offset += Edge0.Num();
				TArray<uint8> Edge1 = { DebuggerData + Offset, Size }; Offset += Edge1.Num();
				States.Add(SpvDebugState_SmoothStep{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.Edge0 = MoveTemp(Edge0),
					.Edge1 = MoveTemp(Edge1)
				});
				break;
			}
			case SpvDebuggerStateType::Pow:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				TArray<uint8> Y = { DebuggerData + Offset, Size }; Offset += Y.Num();
				States.Add(SpvDebugState_Pow{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X),
					.Y = MoveTemp(Y)
				});
				break;
			}
			case SpvDebuggerStateType::Clamp:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> MinVal = { DebuggerData + Offset, Size }; Offset += MinVal.Num();
				TArray<uint8> MaxVal = { DebuggerData + Offset, Size }; Offset += MaxVal.Num();
				States.Add(SpvDebugState_Clamp{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.MinVal = MoveTemp(MinVal),
					.MaxVal = MoveTemp(MaxVal)
				});
				break;
			}
			case SpvDebuggerStateType::Div:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Operand2 = { DebuggerData + Offset, Size }; Offset += Operand2.Num();
				States.Add(SpvDebugState_Div{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.Operand2 = MoveTemp(Operand2),
				});
				break;
			}
			case SpvDebuggerStateType::ConvertF:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> FloatValue = { DebuggerData + Offset, Size }; Offset += FloatValue.Num();
				States.Add(SpvDebugState_ConvertF{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.FloatValue = MoveTemp(FloatValue),
				});
				break;
			}
			case SpvDebuggerStateType::Remainder:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Operand1 = { DebuggerData + Offset, Size }; Offset += Operand1.Num();
				TArray<uint8> Operand2 = { DebuggerData + Offset, Size }; Offset += Operand2.Num();
				States.Add(SpvDebugState_Remainder{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.Operand1 = MoveTemp(Operand1),
					.Operand2 = MoveTemp(Operand2)
				});
				break;
			}
			case SpvDebuggerStateType::Log:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_Log{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Asin:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_Asin{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Acos:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_Acos{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Sqrt:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_Sqrt{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::InverseSqrt:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_InverseSqrt{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Atan2:
			{
				SpvId ResultType = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				int32 Size = GetTypeByteSize(DebuggerContext->Types[ResultType].Get());
				TArray<uint8> Y = { DebuggerData + Offset, Size }; Offset += Y.Num();
				TArray<uint8> X = { DebuggerData + Offset, Size }; Offset += X.Num();
				States.Add(SpvDebugState_Atan2{
					.Line = Line,
					.Source = Source,
					.ResultType = ResultType,
					.Y = MoveTemp(Y),
					.X = MoveTemp(X)
				});
				break;
			}
			case SpvDebuggerStateType::Condition:
			{
				States.Add(SpvDebugState_Tag{
					.Line = Line,
					.Source = Source,
					.bCondition = true,
				});
				break;
			}
			case SpvDebuggerStateType::Return:
			{
				States.Add(SpvDebugState_Tag{
					.Line = Line,
					.Source = Source,
					.bReturn = true,
				});
				break;
			}
			case SpvDebuggerStateType::FuncCall:
			{
				SpvId CallId = *(SpvId*)(DebuggerData + Offset); Offset += 4;
				States.Add(SpvDebugState_FuncCall{
					.Line = Line,
					.Source = Source,
					.CallId = CallId,
				});
				GenCallStack.Add({ Line, Source });
				bJustEnteredCall = true;
				break;
			}
			case SpvDebuggerStateType::Kill:
			{
				States.Add(SpvDebugState_Tag{
					.Line = Line,
					.Source = Source,
					.bKill = true,
				});
				break;
			}
			default:
				AUX::Unreachable();
			}
			// Align offset to 16-byte boundary (uvec4 stride)
			Offset = (Offset + 15) & ~15;
			PackedHeader = *(uint32*)(DebuggerData + Offset);
			UnpackDebugHeader(PackedHeader, StateType, UnpackedSource, UnpackedLine);
		}
		return States;
	}

}
