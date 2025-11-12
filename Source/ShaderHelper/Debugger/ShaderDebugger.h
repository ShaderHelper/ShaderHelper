#pragma once
#include "GpuApi/Spirv/SpirvParser.h"
#include "GpuApi/Spirv/SpirvPixelDebugger.h"
#include "GpuApi/GpuRhi.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDebugger, Log, All);
inline DEFINE_LOG_CATEGORY(LogDebugger);

namespace SH
{
	class SShaderEditorBox;

	struct BindingBuilder
	{
		FW::GpuBindGroupBuilder BingGroupBuilder;
		FW::GpuBindGroupLayoutBuilder LayoutBuilder;
	};

	struct BindingState
	{
		std::optional<BindingBuilder> GlobalBuilder, PassBuilder, ShaderBuilder;
	};

	struct PixelState
	{
		FW::Vector2u Coord;
		FW::GpuViewPortDesc ViewPortDesc;
		BindingState Builders;
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
		void DebugPixel(const BindingState& InBuilders, const PixelState& InState);
		void DebugCompute(const BindingState& InBuilders, const ComputeState& InState);
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
		TArray<uint8> ReturnValue;
		//ValidLine: Line that can trigger a breakpoint
		std::optional<int32> CurValidLine;
		FString DebuggerError;
		int32 CurDebugStateIndex{};
		std::vector<std::pair<FW::SpvId, FW::SpvVariableDesc*>> SortedVariableDescs;
		FW::SpvVariable* AssertResult = nullptr;

		BindingState BindingBuilders;
		FW::SpvDebuggerContext* DebuggerContext = nullptr;
		std::optional<FW::SpvPixelDebuggerContext> PixelDebuggerContext;
		std::optional<PixelState> PsState;

		TArray<FW::SpvDebugState> DebugStates;
	};
}
