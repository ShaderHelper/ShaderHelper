#pragma once
#include "GpuApi/Spirv/SpirvParser.h"
#include "GpuApi/Spirv/SpirvPixelDebugger.h"
#include "GpuApi/Spirv/SpirvPixelPreviewer.h"
#include "GpuApi/GpuRhi.h"
#include "Editor/PreviewViewPort.h"
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

	//Per-line preview info: a line with exactly one uniquely modified variable
	struct LinePreviewInfo
	{
		FW::SpvId VarId;
		int32 IterationIndex;    //How many times this (PackedHeader, VarId, CallStackHash) was seen before (for loop iterations)
		uint32 PackedHeader;     //Packed debug header (StateType|Source|Line)
		FString VarName;
		uint32 CallStackHash;    //Hash of the call stack at the point of this VarChange
	};

	enum class StepMode
	{
		Continue,
		StepOver,
		StepInto,
	};

	struct DebuggerLocation
	{
		FString File;
		int32 LineNumber{};
		
		bool IsValid() const { return !File.IsEmpty() && LineNumber > 0; }
		void Reset() { File.Empty(); LineNumber = 0; }
		bool operator==(const DebuggerLocation& Other) const { return File == Other.File && LineNumber == Other.LineNumber; }
		friend uint32 GetTypeHash(const DebuggerLocation& Loc) { return HashCombine(GetTypeHash(Loc.File), ::GetTypeHash(Loc.LineNumber)); }
	};

	class ShaderDebugger
	{
	public:
		ShaderDebugger();
		static bool EnableUbsan();
		static bool EnableLinePreview();

	public:
		void SetShaderAsset(ShaderAsset* InShaderAsset) { CurShaderAsset = InShaderAsset; }
		ShaderAsset* GetShaderAsset() const { return CurShaderAsset; }
		void SetShaderSource(const FString& InSource) { ShaderSourceText = InSource; }
		
		void ApplyDebugState(const FW::SpvDebugState& InState, FString& Error);
		void ShowDebuggerResult();
		void ShowDebuggerVariable(FW::SpvLexicalScope* InScope) const;
		struct ExpressionNode EvaluateExpression(const FString& InExpression) const;
		bool Continue(StepMode Mode);

		void InvokePixel(bool GlobalValidation = false);
		void DebugPixel(const FW::Vector2u& InPixelCoord, const InvocationState& InState);
		std::optional<FW::Vector2u> ValidatePixel(const InvocationState& InState);

		void DebugCompute(const InvocationState& InState);
		void Reset();

		//Compute per-line preview data and render preview textures
		void ComputeLinePreviewData();
		void InvokePixelPreview();
		const TMap<DebuggerLocation, TRefCountPtr<FW::GpuTexture>>& GetLinePreviewTextures() const { return LinePreviewTextures; }
		const TMap<DebuggerLocation, LinePreviewInfo>& GetLinePreviewData() const { return LinePreviewData; }

		TPair<FString, DebuggerLocation> GetDebuggerError() const { return DebuggerError; }
		const DebuggerLocation& GetStopLocation() const { return StopLocation; }
		bool IsValid() const { return DebuggerContext != nullptr; }
		void MarkEditDuringDebugging() { bEditDuringDebugging = true; }
		bool HasEditDuringDebugging() const { return bEditDuringDebugging; }
		void ClearEditDuringDebugging() { bEditDuringDebugging = false; }
	private:
		int32 GetExtraLineNumForSource(FW::SpvId Source) const;
		int32 GetExtraLineNumForFile(const FString& FileName) const;
		void InitDebuggerView();
		TArray<FW::SpvDebugState> GenDebugStates(uint8* DebuggerData);

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
		DebuggerLocation StopLocation;
		TArray<uint8> ReturnValue;
		//ValidLine: Line that can trigger a breakpoint
		std::optional<int32> CurValidLine;
		TPair<FString, DebuggerLocation> DebuggerError;
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

		//Per-line preview: maps DebuggerLocation -> info/texture
		TMap<DebuggerLocation, LinePreviewInfo> LinePreviewData;
		TSet<DebuggerLocation> DirtyLinePreviewLocs;
		TMap<DebuggerLocation, TRefCountPtr<FW::GpuTexture>> LinePreviewTextures;
		int32 PrevPreviewStateIndex = 0;
		FW::SpvLexicalScope* PrevPreviewScope = nullptr;
		uint32 PrevPreviewCallStackHash = 0;
		TArray<FW::SpvId> PrevPreviewCallIdStack;
	};
}
