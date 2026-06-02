#include "CommonHeader.h"
#include "SVertexDebuggerViewport.h"
#include "Editor/ShaderHelperEditor.h"
#include "Common/Util/Math.h"
#include "GpuApi/GpuRhi.h"

#include <Widgets/SViewport.h>

using namespace FW;

namespace SH
{
	namespace
	{
		const TCHAR GVertexDebuggerWireframeHlsl[] = TEXT(R"(
struct VsInput
{
	float3 Position : POSITION0;
};

float4 MainVS(VsInput Input) : SV_POSITION
{
	return mul(float4(Input.Position, 1.0), Transform);
}

float4 MainPS(float4 Position : SV_POSITION) : SV_Target
{
	return WireColor;
}

struct SolidVsOutput
{
	float4 Position : SV_POSITION;
	float3 WorldPos : TEXCOORD0;
};

SolidVsOutput MainVS_Solid(VsInput Input)
{
	SolidVsOutput Output;
	Output.Position = mul(float4(Input.Position, 1.0), Transform);
	Output.WorldPos = Input.Position;
	return Output;
}

float4 MainPS_Solid(SolidVsOutput Input) : SV_Target
{
	float3 Normal = normalize(cross(ddy(Input.WorldPos), ddx(Input.WorldPos)));
	float3 LightDir = normalize(float3(0.35, -0.65, 0.55));
	float Shade = 0.25 + 0.55 * abs(dot(Normal, LightDir));
	return float4(WireColor.rgb * Shade, 1.0);
}
)");

		const TCHAR GVertexDebuggerOverlayHlsl[] = TEXT(R"(
struct LineVsInput
{
	float3 StartPos : POSITION0;
	float3 EndPos : POSITION1;
	float2 Param : TEXCOORD0;
};

struct PointVsInput
{
	float3 Position : POSITION0;
	float2 Offset : TEXCOORD0;
};

struct PointVsOutput
{
	float4 Position : SV_POSITION;
	float2 Local : TEXCOORD0;
};

float4 MainVS_Line(LineVsInput Input) : SV_POSITION
{
	float4 ClipA = mul(float4(Input.StartPos, 1.0), Transform);
	float4 ClipB = mul(float4(Input.EndPos, 1.0), Transform);
	float2 NdcA = ClipA.xy / max(abs(ClipA.w), 1e-6);
	float2 NdcB = ClipB.xy / max(abs(ClipB.w), 1e-6);
	float2 DirPx = (NdcB - NdcA) * ViewportSize;
	float LenSq = max(dot(DirPx, DirPx), 1e-6);
	float2 NormalPx = float2(-DirPx.y, DirPx.x) * rsqrt(LenSq);
	float4 Clip = lerp(ClipA, ClipB, Input.Param.x);
	float2 OffsetNdc = NormalPx * Input.Param.y * LineThicknessPx * 2.0 / ViewportSize;
	Clip.xy += OffsetNdc * Clip.w;
	return Clip;
}

PointVsOutput MainVS_Point(PointVsInput Input)
{
	PointVsOutput Output;
	Output.Position = mul(float4(Input.Position, 1.0), Transform);
	float2 OffsetNdc = Input.Offset * PointRadiusPx * 2.0 / ViewportSize;
	Output.Position.xy += OffsetNdc * Output.Position.w;
	Output.Local = Input.Offset;
	return Output;
}

float4 MainPS_Overlay(float4 Position : SV_POSITION) : SV_Target
{
	return WireColor;
}

float4 MainPS_Point(PointVsOutput Input) : SV_Target
{
	clip(1.0 - dot(Input.Local, Input.Local));
	return WireColor;
}
)");

		struct OverlayLineVertex
		{
			Vector3f StartPos;
			Vector3f EndPos;
			Vector2f Param;
		};

		struct OverlayPointVertex
		{
			Vector3f Position;
			Vector2f Offset;
		};

		Vector3f GetOrbitCameraPosition(float InDistance, float InYaw, float InPitch)
		{
			const FMatrix44f OrbitRotation = RotationMatrix(InYaw, InPitch);
			const Vector4f OrbitPosition = OrbitRotation.TransformFVector4(FVector4f(0.0f, 0.0f, -InDistance, 1.0f));
			return Vector3f(OrbitPosition.X, OrbitPosition.Y, OrbitPosition.Z);
		}
	}

	void SVertexDebuggerViewport::Construct(const FArguments& InArgs)
	{
		Preview = MakeShared<PreviewViewPort>();

		ViewCamera.Position = { 0.0f, 0.0f, -1.0f };
		ViewCamera.Yaw = -(PI + PI / 8);
		ViewCamera.Pitch = 0.0f;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.AspectRatio = 1.0f;
		ViewCamera.NearPlane = 0.001f;
		ViewCamera.FarPlane = 100000.0f;

		InitRenderResources();

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			[
				SAssignNew(ViewportWidget, SViewport)
				.ViewportInterface(Preview)
			]
		];
		Preview->SetAssociatedWidget(ViewportWidget);

		ResizeHandlerHandle = Preview->ResizeHandler.AddLambda([this](const Vector2f&) {
			Render();
		});

		Preview->MouseDownHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				bRightMouseDown = true;
				LastMousePos = (Vector2f)MouseEvent.GetScreenSpacePosition();
				return FReply::Handled().SetUserFocus(ViewportWidget.ToSharedRef());
			}
			if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				bMiddleMouseDown = true;
				LastMousePos = (Vector2f)MouseEvent.GetScreenSpacePosition();
				return FReply::Handled().SetUserFocus(ViewportWidget.ToSharedRef());
			}
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				bLeftMouseDown = true;
				bLeftDragMoved = false;
				LastMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				return FReply::Handled().SetUserFocus(ViewportWidget.ToSharedRef());
			}
			return FReply::Unhandled();
		});

		Preview->MouseUpHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				bRightMouseDown = false;
				return FReply::Handled();
			}
			if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				bMiddleMouseDown = false;
				return FReply::Handled();
			}
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				const bool bWasClick = bLeftMouseDown && !bLeftDragMoved;
				bLeftMouseDown = false;
				if (bWasClick)
				{
					OnPick();
				}
				return FReply::Handled();
			}
			return FReply::Unhandled();
		});

		Preview->MouseMoveHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
			if (bRightMouseDown && MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
			{
				const Vector2f CurrentMousePos = (Vector2f)MouseEvent.GetScreenSpacePosition();
				const Vector2f MouseDelta = CurrentMousePos - LastMousePos;
				LastMousePos = CurrentMousePos;

				ViewCamera.Yaw += MouseDelta.x * 0.003f;
				ViewCamera.Pitch = FMath::Clamp(ViewCamera.Pitch - MouseDelta.y * 0.003f, -PI * 0.49f, PI * 0.49f);
				Render();
				return FReply::Handled();
			}

			if (bMiddleMouseDown && MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
			{
				const Vector2f CurrentMousePos = (Vector2f)MouseEvent.GetScreenSpacePosition();
				const Vector2f MouseDelta = CurrentMousePos - LastMousePos;
				LastMousePos = CurrentMousePos;

				const FMatrix44f RotMat = ViewCamera.GetWorldRotationMatrix();
				const Vector3f Right(RotMat.M[0][0], RotMat.M[0][1], RotMat.M[0][2]);
				const Vector3f Up(RotMat.M[1][0], RotMat.M[1][1], RotMat.M[1][2]);
				ViewCamera.Position -= Right * MouseDelta.x * 0.01f;
				ViewCamera.Position += Up * MouseDelta.y * 0.01f;
				Render();
				return FReply::Handled();
			}

			if (!bLeftMouseDown || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				return FReply::Unhandled();
			}

			const Vector2f CurrentMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const Vector2f MouseDelta = CurrentMousePos - LastMousePos;
			LastMousePos = CurrentMousePos;
			if (FMath::Abs(MouseDelta.x) + FMath::Abs(MouseDelta.y) > 2.0f)
			{
				bLeftDragMoved = true;
			}
			return FReply::Handled();
		});

		Preview->MouseWheelHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
			const FMatrix44f RotMat = ViewCamera.GetWorldRotationMatrix();
			const Vector3f Forward(RotMat.M[2][0], RotMat.M[2][1], RotMat.M[2][2]);
			ViewCamera.Position += Forward * MouseEvent.GetWheelDelta();
			Render();
			return FReply::Handled();
		});

		KeyDownHandlerHandle = Preview->KeyDownHandler.AddLambda([this](const FGeometry&, const FKeyEvent& KeyEvent) {
			const FKey Key = KeyEvent.GetKey();
			if (Key == EKeys::W) bKeyW = true;
			else if (Key == EKeys::A) bKeyA = true;
			else if (Key == EKeys::S) bKeyS = true;
			else if (Key == EKeys::D) bKeyD = true;
			else if (Key == EKeys::Q) bKeyQ = true;
			else if (Key == EKeys::E) bKeyE = true;
		});
		KeyUpHandlerHandle = Preview->KeyUpHandler.AddLambda([this](const FGeometry&, const FKeyEvent& KeyEvent) {
			const FKey Key = KeyEvent.GetKey();
			if (Key == EKeys::W) bKeyW = false;
			else if (Key == EKeys::A) bKeyA = false;
			else if (Key == EKeys::S) bKeyS = false;
			else if (Key == EKeys::D) bKeyD = false;
			else if (Key == EKeys::Q) bKeyQ = false;
			else if (Key == EKeys::E) bKeyE = false;
		});
		CameraTickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime) {
			UpdateCamera(DeltaTime);
			return true;
		}));
	}

	SVertexDebuggerViewport::~SVertexDebuggerViewport()
	{
		if (CameraTickerHandle.IsValid())
		{
			FTicker::GetCoreTicker().RemoveTicker(CameraTickerHandle);
		}
		if (ResizeHandlerHandle.IsValid())
		{
			Preview->ResizeHandler.Remove(ResizeHandlerHandle);
		}
		if (KeyDownHandlerHandle.IsValid())
		{
			Preview->KeyDownHandler.Remove(KeyDownHandlerHandle);
		}
		if (KeyUpHandlerHandle.IsValid())
		{
			Preview->KeyUpHandler.Remove(KeyUpHandlerHandle);
		}
	}

	void SVertexDebuggerViewport::InitRenderResources()
	{
		WireUbBuilder.AddMatrix4x4f("Transform");
		WireUbBuilder.AddVector4f("WireColor");
		WireUbBuilder.AddVector2f("ViewportSize");
		WireUbBuilder.AddFloat("LineThicknessPx");
		WireUbBuilder.AddFloat("PointRadiusPx");

		WireBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
			.AddUniformBuffer("WireUb", WireUbBuilder, BindingShaderStage::Vertex | BindingShaderStage::Pixel)
			.Build();

		auto MakeUb = [this](const Vector4f& Color)
		{
			TUniquePtr<UniformBuffer> Ub = WireUbBuilder.Build();
			Ub->GetMember<FMatrix44f>("Transform") = FMatrix44f::Identity;
			Ub->GetMember<Vector4f>("WireColor") = Color;
			Ub->GetMember<Vector2f>("ViewportSize") = Vector2f(1.0f, 1.0f);
			Ub->GetMember<float>("LineThicknessPx") = 1.5f;
			Ub->GetMember<float>("PointRadiusPx") = 4.0f;
			return Ub;
		};
		WireUb = MakeUb(Vector4f(0.0f, 1.0f, 0.25f, 1.0f));
		FrustumUb = MakeUb(Vector4f(0.45f, 0.45f, 0.52f, 1.0f));
		HighlightUb = MakeUb(Vector4f(0.20f, 0.55f, 1.0f, 1.0f));
		SolidUb = MakeUb(Vector4f(0.64f, 0.68f, 0.72f, 1.0f));
		HighlightUb->GetMember<float>("PointRadiusPx") = 6.0f;

		auto MakeBindGroup = [this](UniformBuffer* Ub)
		{
			return GpuBindGroupBuilder{ WireBindGroupLayout }
				.SetUniformBuffer("WireUb", Ub->GetGpuResource())
				.Build();
		};
		WireBindGroup = MakeBindGroup(WireUb.Get());
		FrustumBindGroup = MakeBindGroup(FrustumUb.Get());
		HighlightBindGroup = MakeBindGroup(HighlightUb.Get());
		SolidBindGroup = MakeBindGroup(SolidUb.Get());

		FString ShaderSource = WireBindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);
		ShaderSource += GVertexDebuggerWireframeHlsl;
		FString OverlayShaderSource = WireBindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);
		OverlayShaderSource += GVertexDebuggerOverlayHlsl;

		WireVs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerWireVS"), .Source = ShaderSource, .Type = ShaderType::Vertex, .EntryPoint = TEXT("MainVS") });
		WirePs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerWirePS"), .Source = ShaderSource, .Type = ShaderType::Pixel, .EntryPoint = TEXT("MainPS") });
		SolidVs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerSolidVS"), .Source = ShaderSource, .Type = ShaderType::Vertex, .EntryPoint = TEXT("MainVS_Solid") });
		SolidPs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerSolidPS"), .Source = ShaderSource, .Type = ShaderType::Pixel, .EntryPoint = TEXT("MainPS_Solid") });
		OverlayLineVs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerOverlayLineVS"), .Source = OverlayShaderSource, .Type = ShaderType::Vertex, .EntryPoint = TEXT("MainVS_Line") });
		OverlayPointVs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerOverlayPointVS"), .Source = OverlayShaderSource, .Type = ShaderType::Vertex, .EntryPoint = TEXT("MainVS_Point") });
		OverlayPs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerOverlayPS"), .Source = OverlayShaderSource, .Type = ShaderType::Pixel, .EntryPoint = TEXT("MainPS_Overlay") });
		OverlayPointPs = GGpuRhi->CreateShaderFromSource({ .Name = TEXT("VertexDebuggerOverlayPointPS"), .Source = OverlayShaderSource, .Type = ShaderType::Pixel, .EntryPoint = TEXT("MainPS_Point") });

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(WireVs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(WirePs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(SolidVs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(SolidPs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(OverlayLineVs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(OverlayPointVs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(OverlayPs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(OverlayPointPs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());

		const GpuVertexLayoutDesc PositionLayout{
			.ByteStride = sizeof(Vector3f),
			.Attributes = { { .Location = 0, .SemanticName = "POSITION", .Format = GpuFormat::R32G32B32_FLOAT, .ByteOffset = 0 } }
		};
		const GpuVertexLayoutDesc LineLayout{
			.ByteStride = sizeof(OverlayLineVertex),
			.Attributes = {
				{ .Location = 0, .SemanticName = "POSITION", .SemanticIndex = 0, .Format = GpuFormat::R32G32B32_FLOAT, .ByteOffset = offsetof(OverlayLineVertex, StartPos) },
				{ .Location = 1, .SemanticName = "POSITION", .SemanticIndex = 1, .Format = GpuFormat::R32G32B32_FLOAT, .ByteOffset = offsetof(OverlayLineVertex, EndPos) },
				{ .Location = 2, .SemanticName = "TEXCOORD", .SemanticIndex = 0, .Format = GpuFormat::R32G32_FLOAT, .ByteOffset = offsetof(OverlayLineVertex, Param) },
			}
		};
		const GpuVertexLayoutDesc PointLayout{
			.ByteStride = sizeof(OverlayPointVertex),
			.Attributes = {
				{ .Location = 0, .SemanticName = "POSITION", .SemanticIndex = 0, .Format = GpuFormat::R32G32B32_FLOAT, .ByteOffset = offsetof(OverlayPointVertex, Position) },
				{ .Location = 1, .SemanticName = "TEXCOORD", .SemanticIndex = 0, .Format = GpuFormat::R32G32_FLOAT, .ByteOffset = offsetof(OverlayPointVertex, Offset) },
			}
		};

		auto MakePipeline = [&](GpuShader* Vs, GpuShader* Ps, const GpuVertexLayoutDesc& Layout, RasterizerCullMode CullMode, PrimitiveType Primitive, GpuFormat ColorFormat, uint32 SampleCount, bool bDepthWrite, CompareMode DepthCompare)
		{
			GpuRenderPipelineStateDesc Desc{
				.Vs = Vs,
				.Ps = Ps,
				.Targets = { { .TargetFormat = ColorFormat } },
				.BindGroupLayouts = { WireBindGroupLayout },
				.VertexLayout = { Layout },
				.RasterizerState = { .FillMode = RasterizerFillMode::Solid, .CullMode = CullMode },
				.Primitive = Primitive,
				.SampleCount = SampleCount,
				.DepthStencilState = DepthStencilStateDesc{ .DepthFormat = GpuFormat::D32_FLOAT, .DepthWriteEnable = bDepthWrite, .DepthCompare = DepthCompare },
			};
			return GpuPsoCacheManager::Get().CreateRenderPipelineState(Desc);
		};

		SolidPipeline = MakePipeline(SolidVs, SolidPs, PositionLayout, RasterizerCullMode::None, PrimitiveType::TriangleList, GpuFormat::B8G8R8A8_UNORM, 4, true, CompareMode::Less);
		WirePipeline = MakePipeline(OverlayLineVs, OverlayPs, LineLayout, RasterizerCullMode::None, PrimitiveType::TriangleList, GpuFormat::B8G8R8A8_UNORM, 4, false, CompareMode::Always);
		PointPipeline = MakePipeline(OverlayPointVs, OverlayPointPs, PointLayout, RasterizerCullMode::None, PrimitiveType::TriangleList, GpuFormat::B8G8R8A8_UNORM, 4, false, CompareMode::Always);
		FrustumPipeline = MakePipeline(WireVs, WirePs, PositionLayout, RasterizerCullMode::None, PrimitiveType::LineList, GpuFormat::B8G8R8A8_UNORM, 4, false, CompareMode::LessEqual);
	}

	void SVertexDebuggerViewport::SetDebugData(const TArray<Vector4f>& InClipPositions, const TArray<uint32>& InIndices, uint32 InVertexCount, uint32 InInstanceCount, const FMatrix44f& ClipToWorld, const TOptional<Camera>& InDebugCamera)
	{
		SubMeshes.Empty();
		bVertexFinalized = false;
		HighlightVertexBuffer.SafeRelease();
		PerInstanceVertexCount = InVertexCount;
		FrustumVertexBuffer.SafeRelease();

		const Vector3f FrustumCorners[8] = {
			{-1,-1,0}, {1,-1,0}, {1,1,0}, {-1,1,0},
			{-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1},
		};
		const int32 FrustumEdgeIdx[24] = {
			0,1, 1,2, 2,3, 3,0,
			4,5, 5,6, 6,7, 7,4,
			0,4, 1,5, 2,6, 3,7,
		};
		const FMatrix44f FrustumClipToWorld = InDebugCamera.IsSet() ? InDebugCamera.GetValue().GetViewProjectionMatrix().Inverse() : FMatrix44f::Identity;
		TArray<Vector3f> FrustumPositions;
		FrustumPositions.Reserve(24);
		for (int32 i = 0; i < 24; ++i)
		{
			const Vector3f& P = FrustumCorners[FrustumEdgeIdx[i]];
			const Vector4f WorldH = FrustumClipToWorld.TransformFVector4(FVector4f(P.x, P.y, P.z, 1.0f));
			const float W = (FMath::Abs(WorldH.w) > 1e-6f) ? WorldH.w : 1.0f;
			FrustumPositions.Add(Vector3f(WorldH.x / W, WorldH.y / W, WorldH.z / W));
		}
		TArray<uint8> FrustumBytes;
		FrustumBytes.SetNumUninitialized(FrustumPositions.Num() * sizeof(Vector3f));
		FMemory::Memcpy(FrustumBytes.GetData(), FrustumPositions.GetData(), FrustumBytes.Num());
		FrustumVertexBuffer = GGpuRhi->CreateBuffer({ .ByteSize = (uint32)FrustumBytes.Num(), .Usage = GpuBufferUsage::Vertex, .InitialData = FrustumBytes });

		const uint32 InstanceCount = FMath::Max(1u, InInstanceCount);
		if (!InClipPositions.IsEmpty())
		{
			SubMeshRender Sub;
			Sub.WorldPositions.SetNumUninitialized(InClipPositions.Num());
			for (int32 v = 0; v < InClipPositions.Num(); ++v)
			{
				const Vector4f& C = InClipPositions[v];
				const Vector4f WorldH = ClipToWorld.TransformFVector4(FVector4f(C.x, C.y, C.z, C.w));
				const float W = (FMath::Abs(WorldH.w) > 1e-6f) ? WorldH.w : 1.0f;
				Sub.WorldPositions[v] = Vector3f(WorldH.x / W, WorldH.y / W, WorldH.z / W);
			}

			// Replicate the per-instance triangle list across instances, offsetting vertex ids by
			// instance * VertexCount to match the flat capture layout (instance*VertexCount + vertex).
			Sub.Indices.SetNumUninitialized(InIndices.Num() * (int32)InstanceCount);
			for (uint32 Inst = 0; Inst < InstanceCount; ++Inst)
			{
				const uint32 Offset = Inst * InVertexCount;
				const int32 Base = (int32)Inst * InIndices.Num();
				for (int32 k = 0; k < InIndices.Num(); ++k)
				{
					Sub.Indices[Base + k] = InIndices[k] + Offset;
				}
			}
			Sub.IndexCount = (uint32)Sub.Indices.Num();
			Sub.TriangleCount = Sub.IndexCount / 3;

			TArray<uint8> VertexBytes;
			VertexBytes.SetNumUninitialized(Sub.WorldPositions.Num() * sizeof(Vector3f));
			FMemory::Memcpy(VertexBytes.GetData(), Sub.WorldPositions.GetData(), VertexBytes.Num());
			Sub.VertexBuffer = GGpuRhi->CreateBuffer({ .ByteSize = (uint32)VertexBytes.Num(), .Usage = GpuBufferUsage::Vertex, .InitialData = VertexBytes });

			if (Sub.IndexCount > 0)
			{
				TArray<uint8> IndexBytes;
				IndexBytes.SetNumUninitialized(Sub.Indices.Num() * sizeof(uint32));
				FMemory::Memcpy(IndexBytes.GetData(), Sub.Indices.GetData(), IndexBytes.Num());
				Sub.IndexBuffer = GGpuRhi->CreateBuffer({ .ByteSize = (uint32)IndexBytes.Num(), .Usage = GpuBufferUsage::Index, .InitialData = IndexBytes });
			}

			TArray<OverlayLineVertex> OverlayLines;
			OverlayLines.Reserve((int32)Sub.TriangleCount * 18);
			auto AddOverlayLine = [&OverlayLines](const Vector3f& A, const Vector3f& B)
			{
				OverlayLines.Add({ A, B, Vector2f(0.0f, -1.0f) });
				OverlayLines.Add({ A, B, Vector2f(1.0f, -1.0f) });
				OverlayLines.Add({ A, B, Vector2f(1.0f,  1.0f) });
				OverlayLines.Add({ A, B, Vector2f(0.0f, -1.0f) });
				OverlayLines.Add({ A, B, Vector2f(1.0f,  1.0f) });
				OverlayLines.Add({ A, B, Vector2f(0.0f,  1.0f) });
			};
			for (int32 Tri = 0; Tri < (int32)Sub.TriangleCount; ++Tri)
			{
				const int32 I0 = (int32)Sub.Indices[Tri * 3 + 0];
				const int32 I1 = (int32)Sub.Indices[Tri * 3 + 1];
				const int32 I2 = (int32)Sub.Indices[Tri * 3 + 2];
				if (Sub.WorldPositions.IsValidIndex(I0) && Sub.WorldPositions.IsValidIndex(I1) && Sub.WorldPositions.IsValidIndex(I2))
				{
					AddOverlayLine(Sub.WorldPositions[I0], Sub.WorldPositions[I1]);
					AddOverlayLine(Sub.WorldPositions[I1], Sub.WorldPositions[I2]);
					AddOverlayLine(Sub.WorldPositions[I2], Sub.WorldPositions[I0]);
				}
			}
			if (!OverlayLines.IsEmpty())
			{
				TArray<uint8> LineBytes;
				LineBytes.SetNumUninitialized(OverlayLines.Num() * sizeof(OverlayLineVertex));
				FMemory::Memcpy(LineBytes.GetData(), OverlayLines.GetData(), LineBytes.Num());
				Sub.OverlayLineBuffer = GGpuRhi->CreateBuffer({ .ByteSize = (uint32)LineBytes.Num(), .Usage = GpuBufferUsage::Vertex, .InitialData = LineBytes });
				Sub.OverlayLineVertexCount = (uint32)OverlayLines.Num();
			}

			TArray<OverlayPointVertex> OverlayPoints;
			OverlayPoints.Reserve(Sub.WorldPositions.Num() * 6);
			const Vector2f PointOffsets[6] = {
				{-1.0f, -1.0f}, { 1.0f, -1.0f}, { 1.0f,  1.0f},
				{-1.0f, -1.0f}, { 1.0f,  1.0f}, {-1.0f,  1.0f},
			};
			for (const Vector3f& P : Sub.WorldPositions)
			{
				for (const Vector2f& Offset : PointOffsets)
				{
					OverlayPoints.Add({ P, Offset });
				}
			}
			if (!OverlayPoints.IsEmpty())
			{
				TArray<uint8> PointBytes;
				PointBytes.SetNumUninitialized(OverlayPoints.Num() * sizeof(OverlayPointVertex));
				FMemory::Memcpy(PointBytes.GetData(), OverlayPoints.GetData(), PointBytes.Num());
				Sub.OverlayPointBuffer = GGpuRhi->CreateBuffer({ .ByteSize = (uint32)PointBytes.Num(), .Usage = GpuBufferUsage::Vertex, .InitialData = PointBytes });
				Sub.OverlayPointVertexCount = (uint32)OverlayPoints.Num();
			}

			SubMeshes.Add(MoveTemp(Sub));
		}

		FrameCamera(InDebugCamera, FrustumPositions);
		Render();
	}

	void SVertexDebuggerViewport::FrameCamera(const TOptional<Camera>& InDebugCamera, const TArray<Vector3f>& InFrustumPositions)
	{
		bUseDebugCameraAspect = InDebugCamera.IsSet();

		Vector3f Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		bool bAny = false;
		for (const SubMeshRender& Sub : SubMeshes)
		{
			for (const Vector3f& P : Sub.WorldPositions)
			{
				Min = Vector3f(FMath::Min(Min.x, P.x), FMath::Min(Min.y, P.y), FMath::Min(Min.z, P.z));
				Max = Vector3f(FMath::Max(Max.x, P.x), FMath::Max(Max.y, P.y), FMath::Max(Max.z, P.z));
				bAny = true;
			}
		}
		for (const Vector3f& P : InFrustumPositions)
		{
			Min = Vector3f(FMath::Min(Min.x, P.x), FMath::Min(Min.y, P.y), FMath::Min(Min.z, P.z));
			Max = Vector3f(FMath::Max(Max.x, P.x), FMath::Max(Max.y, P.y), FMath::Max(Max.z, P.z));
			bAny = true;
		}
		if (!bAny)
		{
			SceneCenter = Vector3f(0, 0, 0);
			SceneRadius = 1.0f;
		}
		else
		{
			SceneCenter = (Min + Max) * 0.5f;
			SceneRadius = FMath::Max(FVector3f(Max - SceneCenter).Size(), 0.01f);
		}

		if (InDebugCamera.IsSet())
		{
			ViewCamera = InDebugCamera.GetValue();
			ViewCamera.NearPlane = 0.001f;
			ViewCamera.FarPlane = 100000.0f;
			return;
		}

		ViewCamera.Yaw = 0.0f;
		ViewCamera.Pitch = 0.0f;
		ViewCamera.Roll = 0.0f;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.bOrthographic = false;
		CameraDistance = SceneRadius / FMath::Tan(ViewCamera.VerticalFov * 0.5f) * 1.6f;
		MinCameraDistance = FMath::Max(SceneRadius * 0.05f, 0.02f);
		MaxCameraDistance = FMath::Max(CameraDistance * 8.0f, MinCameraDistance + 1.0f);
		UpdateDefaultOrbitCamera(CameraDistance);
	}

	void SVertexDebuggerViewport::UpdateDefaultOrbitCamera(float InDistance)
	{
		CameraDistance = FMath::Clamp(InDistance, MinCameraDistance, MaxCameraDistance);
		ViewCamera.Position = SceneCenter + GetOrbitCameraPosition(CameraDistance, ViewCamera.Yaw, ViewCamera.Pitch);
		ViewCamera.NearPlane = 0.001f;
		ViewCamera.FarPlane = 100000.0f;
	}

	void SVertexDebuggerViewport::UpdateCamera(float DeltaTime)
	{
		if (!bRightMouseDown)
		{
			return;
		}

		const FMatrix44f RotMat = ViewCamera.GetWorldRotationMatrix();
		const Vector3f Forward(RotMat.M[2][0], RotMat.M[2][1], RotMat.M[2][2]);
		const Vector3f Right(RotMat.M[0][0], RotMat.M[0][1], RotMat.M[0][2]);
		const Vector3f Up(0.0f, 1.0f, 0.0f);

		Vector3f MoveDir(0.0f);
		if (bKeyW) MoveDir += Forward;
		if (bKeyS) MoveDir -= Forward;
		if (bKeyD) MoveDir += Right;
		if (bKeyA) MoveDir -= Right;
		if (bKeyE) MoveDir += Up;
		if (bKeyQ) MoveDir -= Up;

		if (MoveDir.X == 0.0f && MoveDir.Y == 0.0f && MoveDir.Z == 0.0f)
		{
			return;
		}

		const float Len = FMath::Sqrt(MoveDir.X * MoveDir.X + MoveDir.Y * MoveDir.Y + MoveDir.Z * MoveDir.Z);
		if (Len <= 1e-6f)
		{
			return;
		}

		ViewCamera.Position += MoveDir * (CameraMoveSpeed * DeltaTime / Len);
		Render();
	}

	void SVertexDebuggerViewport::ResizeRenderTargetIfNeeded()
	{
		const FIntPoint ViewportSize = Preview->GetSize();
		const uint32 Width = FMath::Max(1, ViewportSize.X);
		const uint32 Height = FMath::Max(1, ViewportSize.Y);
		if (RenderTarget.IsValid() && RenderTarget->GetWidth() == Width && RenderTarget->GetHeight() == Height)
		{
			return;
		}

		GpuTextureDesc ColorDesc{ .Width = Width, .Height = Height, .Format = GpuFormat::B8G8R8A8_UNORM,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared, .ClearValues = Vector4f(0.04f, 0.04f, 0.05f, 1.0f) };
		RenderTarget = GGpuRhi->CreateTexture(ColorDesc);

		GpuTextureDesc MsaaDesc = ColorDesc;
		MsaaDesc.Usage = GpuTextureUsage::RenderTarget;
		MsaaDesc.SampleCount = 4;
		MsaaRenderTarget = GGpuRhi->CreateTexture(MsaaDesc);

		DepthTarget = GGpuRhi->CreateTexture({ .Width = Width, .Height = Height, .Format = GpuFormat::D32_FLOAT,
			.Usage = GpuTextureUsage::DepthStencil, .SampleCount = 4 });

		if (!bUseDebugCameraAspect)
		{
			ViewCamera.AspectRatio = (float)Width / (float)Height;
		}
	}

	void SVertexDebuggerViewport::Render()
	{
		ResizeRenderTargetIfNeeded();
		if (SubMeshes.IsEmpty())
		{
			Preview->Clear();
			return;
		}

		const FMatrix44f Transform = ViewCamera.GetViewProjectionMatrix();
		WireUb->GetMember<FMatrix44f>("Transform") = Transform;
		FrustumUb->GetMember<FMatrix44f>("Transform") = Transform;
		HighlightUb->GetMember<FMatrix44f>("Transform") = Transform;
		SolidUb->GetMember<FMatrix44f>("Transform") = Transform;
		const FIntPoint ViewSize = Preview->GetSize();
		const Vector2f ViewportSize((float)FMath::Max(1, ViewSize.X), (float)FMath::Max(1, ViewSize.Y));
		WireUb->GetMember<Vector2f>("ViewportSize") = ViewportSize;
		FrustumUb->GetMember<Vector2f>("ViewportSize") = ViewportSize;
		HighlightUb->GetMember<Vector2f>("ViewportSize") = ViewportSize;
		SolidUb->GetMember<Vector2f>("ViewportSize") = ViewportSize;

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaRenderTarget->GetDefaultView(), RenderTargetLoadAction::Clear, RenderTargetStoreAction::DontCare,
			Vector4f(0.04f, 0.04f, 0.05f, 1.0f), RenderTarget->GetDefaultView() });
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{ DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear, RenderTargetStoreAction::DontCare, 1.0f };

		auto CmdRecorder = GGpuRhi->BeginRecording("VertexDebuggerView");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "VertexDebuggerView");
			{
				PassRecorder->SetRenderPipelineState(SolidPipeline);
				PassRecorder->SetBindGroups({ SolidBindGroup });
				for (const SubMeshRender& Sub : SubMeshes)
				{
					PassRecorder->SetVertexBuffer(0, Sub.VertexBuffer);
					if (Sub.IndexBuffer)
					{
						PassRecorder->SetIndexBuffer(Sub.IndexBuffer);
						PassRecorder->DrawIndexed(0, Sub.IndexCount, 0, 0, 1);
					}
					else
					{
						PassRecorder->DrawPrimitive(0, Sub.WorldPositions.Num(), 0, 1);
					}
				}

				if (FrustumVertexBuffer)
				{
					PassRecorder->SetRenderPipelineState(FrustumPipeline);
					PassRecorder->SetBindGroups({ FrustumBindGroup });
					PassRecorder->SetVertexBuffer(0, FrustumVertexBuffer);
					PassRecorder->DrawPrimitive(0, 24, 0, 1);
				}

				// Thick green overlay edges and vertex dots.
				PassRecorder->SetRenderPipelineState(WirePipeline);
				PassRecorder->SetBindGroups({ WireBindGroup });
				for (const SubMeshRender& Sub : SubMeshes)
				{
					if (Sub.OverlayLineBuffer && Sub.OverlayLineVertexCount > 0)
					{
						PassRecorder->SetVertexBuffer(0, Sub.OverlayLineBuffer);
						PassRecorder->DrawPrimitive(0, Sub.OverlayLineVertexCount, 0, 1);
					}
				}

				PassRecorder->SetRenderPipelineState(PointPipeline);
				PassRecorder->SetBindGroups({ WireBindGroup });
				for (const SubMeshRender& Sub : SubMeshes)
				{
					if (Sub.OverlayPointBuffer && Sub.OverlayPointVertexCount > 0)
					{
						PassRecorder->SetVertexBuffer(0, Sub.OverlayPointBuffer);
						PassRecorder->DrawPrimitive(0, Sub.OverlayPointVertexCount, 0, 1);
					}
				}

				if (HighlightVertexBuffer)
				{
					PassRecorder->SetRenderPipelineState(PointPipeline);
					PassRecorder->SetBindGroups({ HighlightBindGroup });
					PassRecorder->SetVertexBuffer(0, HighlightVertexBuffer);
					PassRecorder->DrawPrimitive(0, 6, 0, 1);
				}
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });

		Preview->SetViewPortRenderTexture(RenderTarget);
	}

	void SVertexDebuggerViewport::OnPick()
	{
		if (bVertexFinalized)
		{
			return;
		}

		const Vector2f Pixel = Preview->GetMousePos();
		const FIntPoint ViewSize = Preview->GetSize();
		const float Width = (float)FMath::Max(1, ViewSize.X);
		const float Height = (float)FMath::Max(1, ViewSize.Y);
		const FMatrix44f Transform = ViewCamera.GetViewProjectionMatrix();
		constexpr float PickRadiusPx = 8.0f;
		const float PickRadiusSq = PickRadiusPx * PickRadiusPx;

		int32 BestSubMesh = -1;
		int32 BestVertex = -1;
		float BestDistSq = PickRadiusSq;
		float BestDepth = FLT_MAX;

		for (int32 SubMeshIndex = 0; SubMeshIndex < SubMeshes.Num(); ++SubMeshIndex)
		{
			const SubMeshRender& Sub = SubMeshes[SubMeshIndex];
			for (int32 VertexIndex = 0; VertexIndex < Sub.WorldPositions.Num(); ++VertexIndex)
			{
				const Vector3f& P = Sub.WorldPositions[VertexIndex];
				const FVector4f Clip = Transform.TransformFVector4(FVector4f(P.x, P.y, P.z, 1.0f));
				if (Clip.W <= 1e-6f)
				{
					continue;
				}
				const float NdcX = Clip.X / Clip.W;
				const float NdcY = Clip.Y / Clip.W;
				const float NdcZ = Clip.Z / Clip.W;
				if (NdcX < -1.0f || NdcX > 1.0f || NdcY < -1.0f || NdcY > 1.0f || NdcZ < 0.0f || NdcZ > 1.0f)
				{
					continue;
				}

				const Vector2f Screen((NdcX * 0.5f + 0.5f) * Width, (1.0f - (NdcY * 0.5f + 0.5f)) * Height);
				const float Dx = Screen.x - Pixel.x;
				const float Dy = Screen.y - Pixel.y;
				const float DistSq = Dx * Dx + Dy * Dy;
				if (DistSq <= BestDistSq && (DistSq < BestDistSq || NdcZ < BestDepth))
				{
					BestSubMesh = SubMeshIndex;
					BestVertex = VertexIndex;
					BestDistSq = DistSq;
					BestDepth = NdcZ;
				}
			}
		}

		if (BestSubMesh < 0 || BestVertex < 0)
		{
			return;
		}

		const SubMeshRender& Sub = SubMeshes[BestSubMesh];
		const Vector3f VertexPos = Sub.WorldPositions[BestVertex];
		const OverlayPointVertex MarkerVerts[6] = {
			{ VertexPos, Vector2f(-1.0f, -1.0f) }, { VertexPos, Vector2f( 1.0f, -1.0f) }, { VertexPos, Vector2f( 1.0f,  1.0f) },
			{ VertexPos, Vector2f(-1.0f, -1.0f) }, { VertexPos, Vector2f( 1.0f,  1.0f) }, { VertexPos, Vector2f(-1.0f,  1.0f) },
		};
		TArray<uint8> MarkerBytes;
		MarkerBytes.SetNumUninitialized(sizeof(MarkerVerts));
		FMemory::Memcpy(MarkerBytes.GetData(), MarkerVerts, sizeof(MarkerVerts));
		HighlightVertexBuffer = GGpuRhi->CreateBuffer({ .ByteSize = (uint32)MarkerBytes.Num(), .Usage = GpuBufferUsage::Vertex, .InitialData = MarkerBytes });

		Render();

		// BestVertex is the flat id (instance*VertexCount + localVertex);
		const uint32 VtxPerInstance = FMath::Max(1u, PerInstanceVertexCount);
		const uint32 LocalVertex = (uint32)BestVertex % VtxPerInstance;
		const uint32 Instance = (uint32)BestVertex / VtxPerInstance;
		bVertexFinalized = true;
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->DebugVertex(LocalVertex, Instance);
	}

	void SVertexDebuggerViewport::Clear()
	{
		SubMeshes.Empty();
		bUseDebugCameraAspect = false;
		bVertexFinalized = false;
		HighlightVertexBuffer.SafeRelease();
		FrustumVertexBuffer.SafeRelease();
		Preview->Clear();
	}
}
