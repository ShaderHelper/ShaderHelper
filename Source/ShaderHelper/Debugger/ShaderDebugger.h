#pragma once
#include "GpuApi/Spirv/SpirvParser.h"
#include "GpuApi/Spirv/SpirvPixelDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace SH
{
	class SShaderEditorBox;

	struct BindingBuilder
	{
		FW::GpuBindGroupBuilder BingGroupBuilder;
		FW::GpuBindGroupLayoutBuilder LayoutBuilder;
	};

	struct PixelState
	{
		FW::Vector2u Coord;
		FW::GpuViewPortDesc ViewPortDesc;
		std::optional<BindingBuilder> GlobalBuilder, PassBuilder, ShaderBuilder;
		FW::GpuRenderPipelineStateDesc PipelineDesc;
		TFunction<void(FW::GpuRenderPassRecorder*)> DrawFunction;
	};

	struct ComputeState
	{

	};

	enum class StepMode
	{
		Continue,
		StepOver,
		StepInto,
	};

	class ShaderDebugger
	{
	public:
		ShaderDebugger(SShaderEditorBox* InShaderEditor);

	public:
		void ApplyDebugState(const FW::SpvDebugState& InState);
		void ShowDebuggerResult() const;
		void ShowDeuggerVariable(FW::SpvLexicalScope* InScope) const;
		struct ExpressionNode EvaluateExpression(const FString& InExpression) const;
		bool Continue(StepMode Mode);
		void DebugPixel(const PixelState& InState);
		void DebugCompute(const ComputeState& InState);
		void Reset();

		FString GetDebuggerError() const { return DebuggerError; }
		int32 GetStopLineNumber() const { return StopLineNumber; }
		bool IsValid() const { return DebuggerContext != nullptr; }
	private:
		void InitDebuggerView();
		void GenDebugStates(uint8* DebuggerData);

	private:
		SShaderEditorBox* ShaderEditor;
		TArray<FW::ShaderFunc> Funcs;
		TArray<TPair<FW::SpvLexicalScope*, int>> CallStack;
		FW::SpvLexicalScope* Scope = nullptr;
		FW::SpvLexicalScope* CallStackScope = nullptr;
		TMultiMap<FW::SpvId, FW::SpvVarDirtyRange> DirtyVars;
		int32 StopLineNumber{};
		//ValidLine: Line that can trigger a breakpoint
		std::optional<int32> CurValidLine;
		FString DebuggerError;
		int32 CurDebugStateIndex{};
		FW::SpvDebuggerContext* DebuggerContext = nullptr;
		std::vector<std::pair<FW::SpvId, FW::SpvVariableDesc*>> SortedVariableDescs;
		FW::SpvVariable* AssertResult = nullptr;

		std::optional<FW::SpvPixelDebuggerContext> PixelDebuggerContext;

		TArray<FW::SpvDebugState> DebugStates;
	};
}
