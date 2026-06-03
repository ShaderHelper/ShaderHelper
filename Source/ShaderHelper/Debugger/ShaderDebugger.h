#pragma once
#include "GpuApi/Spirv/SpirvDebugger.h"
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Camera.h"


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

	struct VertexState
	{
		FW::GpuViewPortDesc ViewPortDesc;
		TArray<BindingBuilder> Builders;
		FW::GpuRenderPipelineStateDesc PipelineDesc;
		TFunction<void(FW::GpuRenderPassRecorder*)> DrawFunction;
		uint32 VertexCount = 0;   // per-instance vertex count
		uint32 InstanceCount = 1;
		TArray<uint32> Indices;   // per-instance triangle index list (for the geometry viewer / picking)
		FMatrix44f ClipToWorld = FMatrix44f::Identity;
		TOptional<FW::Camera> DebugCamera;
	};

	struct PixelState
	{
		FW::GpuViewPortDesc ViewPortDesc;
		TArray<BindingBuilder> Builders;
		FW::GpuRenderPipelineStateDesc PipelineDesc;
		TFunction<void(FW::GpuRenderPassRecorder*)> DrawFunction;
	};

	struct ComputeState
	{
		FW::Vector3u ThreadGroupCount{ 1, 1, 1 };
		FW::Vector3u ThreadGroupSize{ 1, 1, 1 };
		TArray<BindingBuilder> Builders;
		FW::GpuComputePipelineStateDesc PipelineDesc;
	};

	using InvocationState = std::variant<VertexState, PixelState, ComputeState>;

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

		void Reset();

		//Vertex
		// Returns the captured clip-space positions, indexed by vertex id.
		TArray<FW::Vector4f> CaptureVertex(const InvocationState& InState);
		void InvokeVertex(bool GlobalValidation = false);
		void DebugVertex(uint32 InVertexIndex, uint32 InInstanceIndex, const InvocationState& InState);
		std::optional<FW::Vector2u> ValidateVertex(const InvocationState& InState);
		//

		//Pixel
		void InvokePixel(bool GlobalValidation = false);
		void DebugPixel(const FW::Vector2u& InPixelCoord, const InvocationState& InState);
		std::optional<FW::Vector2u> ValidatePixel(const InvocationState& InState);
		void ComputeLinePreviewData();//Compute per-line preview data and render preview textures
		void InvokePixelPreview();
		const TMap<DebuggerLocation, TRefCountPtr<FW::GpuTexture>>& GetLinePreviewTextures() const { return LinePreviewTextures; }
		const TMap<DebuggerLocation, LinePreviewInfo>& GetLinePreviewData() const { return LinePreviewData; }
		//

		//Compute
		void InvokeCompute(bool GlobalValidation = false);
		bool CanThreadReachStop(uint32 LocalLinearIndex) const;
		void DebugCompute(const FW::Vector3u& InWorkGroupId, const FW::Vector3u& InLocalInvocationId, const InvocationState& InState);
		std::optional<TPair<FW::Vector3u, FW::Vector3u>> ValidateCompute(const InvocationState& InState);
		bool SwitchDebugThread(const FW::Vector3u& InLocalInvocationId);
		//

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
		void FinalizeDebugStates();
		void UpdateThreadsReachingStop();
		bool MatchesStopLocation(const FW::SpvDebugState& InState, const DebuggerLocation& InLoc) const;
		void SaveDebuggerContextState();
		void RestoreDebuggerContextState();
		TArray<FW::SpvDebugState> GenDebugStates(uint8* DebuggerData, uint32 DebuggerDataSize);
		void RebuildComputeWorkgroupStates();
		TArray<FW::SpvDebugState> BuildComputeDebugStatesForThread(uint32 LocalLinearIndex) const;

		bool EvaluateExpressionVertexlImpl(const FString& InExpression, struct ExpressionNode& OutResult) const;
		bool EvaluateExpressionPixelImpl(const FString& InExpression, struct ExpressionNode& OutResult) const;
		bool EvaluateExpressionComputeImpl(const FString& InExpression, struct ExpressionNode& OutResult) const;
		void BuildPatchedBindings(const TArray<BindingBuilder>& Builders, const TArray<FW::GpuShaderLayoutBinding>& LayoutBindings, FW::BindingShaderStage Stage, bool bIncludeParamsBinding);

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
		struct DebuggerVariableState
		{
			std::variant<FW::SpvObject::External, FW::SpvObject::Internal> Storage;
			TArray<FW::Vector2i> InitializedRanges;
		};
		TMap<FW::SpvId, DebuggerVariableState> InitialGlobalVariableStates;
		TMap<FW::SpvId, DebuggerVariableState> InitialLocalVariableStates;
		TArray<TRefCountPtr<FW::GpuBindGroup>> PatchedBindGroups;
		TArray<TRefCountPtr<FW::GpuBindGroupLayout>> PatchedBindGroupLayouts;
		TArray<TRefCountPtr<FW::GpuBindGroup>> SetupBindGroups;
		TArray<TRefCountPtr<FW::GpuBindGroupLayout>> SetupBindGroupLayouts;
		TUniquePtr<FW::SpvDebuggerContext> DebuggerContext;
		InvocationState Invocation;
		TRefCountPtr<FW::GpuShader> DebugShader;
		TArray<FW::SpvDebugState> DebugStates;
		TRefCountPtr<FW::GpuBuffer> DebugBuffer;
		TRefCountPtr<FW::GpuBuffer> DebugParamsBuffer;
		bool bEnableUbsan = false;
		bool bEditDuringDebugging = false;

		//Vertex
		uint32 TargetVertexIndex{};
		uint32 TargetInstanceIndex{};

		//Pixel
		FW::Vector2u PixelCoord;
		//Per-line preview: maps DebuggerLocation -> info/texture
		TMap<DebuggerLocation, LinePreviewInfo> LinePreviewData;
		TSet<DebuggerLocation> DirtyLinePreviewLocs;
		TMap<DebuggerLocation, TRefCountPtr<FW::GpuTexture>> LinePreviewTextures;
		int32 PrevPreviewStateIndex = 0;
		FW::SpvLexicalScope* PrevPreviewScope = nullptr;
		uint32 PrevPreviewCallStackHash = 0;
		TArray<FW::SpvId> PrevPreviewCallIdStack;
		//

		//Compute
		FW::Vector3u WorkGroupId;
		FW::Vector3u LocalInvocationId;
		FW::Vector3u DebuggingThreadGroupSize{ 1, 1, 1 };
		TArray<TArray<FW::SpvDebugState>> PerThreadDebugStates;
		TArray<TArray<FW::SpvDebugState>> ReconstructedWorkgroupStatesByBarrier;
		TSet<uint64> InvalidWorkgroupBarrierKeys;
		int32 StopIterationIndex{};
		TBitArray<> ThreadsReachingStop;
		//
	};
}
