#pragma once
#include "GpuApi/Spirv/SpirvParser.h"
#include "GpuApi/Spirv/SpirvPixelDebugger.h"
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Shader/Shader.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDebugger, Log, All);
inline DEFINE_LOG_CATEGORY(LogDebugger);

namespace SH
{
	class SShaderEditorBox;
	class ShaderAsset;

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
		FW::GpuViewPortDesc ViewPortDesc;
		BindingState Builders;
		FW::GpuRenderPipelineStateDesc PipelineDesc;
		TFunction<void(FW::GpuRenderPassRecorder*)> DrawFunction;
	};

	struct ComputeState
	{

	};

	using InvocationState = std::variant<PixelState, ComputeState>;

	enum class StepMode
	{
		Continue,
		StepOver,
		StepInto,
	};

	class ShaderDebugger
	{
	public:
		ShaderDebugger();
		static bool EnableUbsan();

	public:
		void SetShaderAsset(ShaderAsset* InShaderAsset) { CurShaderAsset = InShaderAsset; }
		ShaderAsset* GetShaderAsset() const { return CurShaderAsset; }
		void SetShaderSource(const FString& InSource) { ShaderSourceText = InSource; }
		
		void ApplyDebugState(const FW::SpvDebugState& InState, FString& Error);
		void ShowDebuggerResult();
		void ShowDeuggerVariable(FW::SpvLexicalScope* InScope) const;
		struct ExpressionNode EvaluateExpression(const FString& InExpression) const;
		bool Continue(StepMode Mode);

		void InvokePixel(bool GlobalValidation = false);
		void DebugPixel(const FW::Vector2u& InPixelCoord, const InvocationState& InState);
		std::optional<FW::Vector2u> ValidatePixel(const InvocationState& InState);

		void DebugCompute(const InvocationState& InState);
		void Reset();

		TPair<FString, int> GetDebuggerError() const { return DebuggerError; }
		int32 GetStopLineNumber() const { return StopLineNumber; }
		FString GetStopFile() const { return StopFile; }
		bool IsValid() const { return DebuggerContext != nullptr; }
		void MarkEditDuringDebugging() { bEditDuringDebugging = true; }
		bool HasEditDuringDebugging() const { return bEditDuringDebugging; }
		void ClearEditDuringDebugging() { bEditDuringDebugging = false; }
	private:
		int32 GetExtraLineNumForSource(FW::SpvId Source) const;
		void InitDebuggerView();
		void GenDebugStates(uint8* DebuggerData);

	private:
		ShaderAsset* CurShaderAsset = nullptr;
		FString ShaderSourceText;
		struct FuncCallPoint
		{
			FW::SpvLexicalScope* Scope{};
			int Line{};
			FString File;
			int DebugStateIndex{};
			const FW::SpvFuncCall* Call{};
		};
		TArray<FuncCallPoint> CallStack;
		FW::SpvLexicalScope* Scope = nullptr;
		std::optional<FuncCallPoint> ActiveCallPoint;
		FW::SpvVariable* AssertResult = nullptr;
		TMultiMap<FW::SpvId, FW::SpvVarDirtyRange> DirtyVars;
		int32 StopLineNumber{};
		FString StopFile;
		TArray<uint8> ReturnValue;
		//ValidLine: Line that can trigger a breakpoint
		std::optional<int32> CurValidLine;
		TPair<FString, int> DebuggerError;
		int32 CurDebugStateIndex{};
		std::vector<std::pair<FW::SpvId, FW::SpvVariableDesc*>> SortedVariableDescs;

		TArray<FW::SpvBinding> SpvBindings;
		FW::BindingContext PatchedBindings;
		TUniquePtr<FW::SpvDebuggerContext> DebuggerContext;
		InvocationState Invocation;
		TRefCountPtr<FW::GpuShader> DebugShader;
		TArray<FW::SpvDebugState> DebugStates;
		TRefCountPtr<FW::GpuBuffer> DebugBuffer;
		TRefCountPtr<FW::GpuBuffer> DebugParamsBuffer;
		bool bEnableUbsan = false;
		bool bEditDuringDebugging = false;

		FW::Vector2u PixelCoord;
	};
}
