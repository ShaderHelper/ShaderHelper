#include "CommonHeader.h"
#include "MaterialPreviewRenderer.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuBindGroup.h"
#include "GpuApi/GpuResourceHelper.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture3D.h"
#include "RenderResource/Camera.h"
#include "RenderResource/UniformBuffer.h"
#include "Common/Util/Math.h"

#include <stdexcept>

using namespace FW;

namespace SH
{
	namespace
	{
		constexpr uint32 PreviewSampleCount = 4;
		constexpr GpuFormat PreviewColorFormat = GpuFormat::B8G8R8A8_UNORM;
		constexpr GpuFormat PreviewDepthFormat = GpuFormat::D32_FLOAT;
		const Vector4f PreviewClearColor(0.08f, 0.08f, 0.08f, 1.0f);

		TUniquePtr<UniformBuffer> BuildUniformBufferFromReflection(const TArray<GpuShaderUbMemberInfo>& Members)
		{
			UniformBufferMetaData MetaData;
			uint32 MaxEnd = 0;
			for (const auto& Member : Members)
			{
				FString HlslType, GlslType;
				if (IsShaderMatrix4x4Type(Member.Type))                                                        { HlslType = TEXT("float4x4"); GlslType = TEXT("mat4"); }
				else if (IsShaderVector4Type(Member.Type))                                                     { HlslType = TEXT("float4"); GlslType = TEXT("vec4"); }
				else if (IsShaderVector3Type(Member.Type))                                                     { HlslType = TEXT("float3"); GlslType = TEXT("vec3"); }
				else if (IsShaderVector2Type(Member.Type))                                                     { HlslType = TEXT("float2"); GlslType = TEXT("vec2"); }
				else if (Member.Type == TEXT("float"))                                                         { HlslType = TEXT("float"); GlslType = TEXT("float"); }
				else if (Member.Type == TEXT("int") || IsShaderBoolType(Member.Type))                          { HlslType = TEXT("int"); GlslType = TEXT("int"); }
				else if (Member.Type == TEXT("uint"))                                                          { HlslType = TEXT("uint"); GlslType = TEXT("uint"); }
				else if (IsShaderIntVector4Type(Member.Type) || IsShaderBoolVector4Type(Member.Type))           { HlslType = TEXT("int4"); GlslType = TEXT("ivec4"); }
				else if (IsShaderIntVector3Type(Member.Type) || IsShaderBoolVector3Type(Member.Type))           { HlslType = TEXT("int3"); GlslType = TEXT("ivec3"); }
				else if (IsShaderIntVector2Type(Member.Type) || IsShaderBoolVector2Type(Member.Type))           { HlslType = TEXT("int2"); GlslType = TEXT("ivec2"); }
				else if (IsShaderUintVector4Type(Member.Type))                                                 { HlslType = TEXT("uint4"); GlslType = TEXT("uvec4"); }
				else if (IsShaderUintVector3Type(Member.Type))                                                 { HlslType = TEXT("uint3"); GlslType = TEXT("uvec3"); }
				else if (IsShaderUintVector2Type(Member.Type))                                                 { HlslType = TEXT("uint2"); GlslType = TEXT("uvec2"); }
				else                                                                                          { HlslType = Member.Type; GlslType = Member.Type; }

				MetaData.Members.Add(Member.Name, { Member.Offset, Member.Size, HlslType, GlslType });
				MaxEnd = FMath::Max(MaxEnd, Member.Offset + Member.Size);
			}
			MetaData.UniformBufferSize = Align(MaxEnd, 16u);
			if (MetaData.UniformBufferSize > 0)
			{
				TRefCountPtr<GpuBuffer> Buffer = GGpuRhi->CreateBuffer({ MetaData.UniformBufferSize, GpuBufferUsage::Uniform });
				return MakeUnique<UniformBuffer>(MoveTemp(Buffer), MoveTemp(MetaData));
			}
			return {};
		}
	}

	MaterialPreviewRenderer::MaterialPreviewRenderer(Material* InMaterial)
		: MaterialAsset(InMaterial)
	{
		check(MaterialAsset);
	}

	void MaterialPreviewRenderer::ResetRenderContext()
	{
		Pipeline = nullptr;
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		PreviewUniformBuffers.Empty();
		LinkageErrorFunc = nullptr;
	}

	void MaterialPreviewRenderer::SetPreviewPrimitive(MaterialPreviewPrimitive InPreviewPrimitive)
	{
		if (PreviewPrimitive == InPreviewPrimitive)
		{
			return;
		}

		PreviewPrimitive = InPreviewPrimitive;
		bPreviewMeshInitialized = false;
	}

	void MaterialPreviewRenderer::SetOrbit(float InYaw, float InPitch)
	{
		if (OrbitYaw == InYaw && OrbitPitch == InPitch)
		{
			return;
		}

		OrbitYaw = InYaw;
		OrbitPitch = InPitch;
	}

	void MaterialPreviewRenderer::SetCameraDistance(float InDistance)
	{
		CameraDistance = FMath::Clamp(InDistance, 1.0f, 10.0f);
	}

	bool MaterialPreviewRenderer::Render(uint32 InWidth, uint32 InHeight)
	{
		ResizeRenderTargetsIfNeeded(InWidth, InHeight);
		if (!EnsurePipeline())
		{
			return false;
		}

		if (!EnsurePreviewMesh())
		{
			return false;
		}

		UpdatePreviewTransform(InWidth, InHeight);

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaRenderTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			PreviewClearColor,
			RenderTarget->GetDefaultView()
		});
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
			DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			1.0f
		};

		auto CmdRecorder = GGpuRhi->BeginRecording("MaterialPreview");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "MaterialPreview");
			{
				TArray<GpuBindGroup*> BindGroupArray;
				for (auto& [_, BindGroup] : BindGroups)
				{
					BindGroupArray.Add(BindGroup.GetReference());
				}

				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups(BindGroupArray);
				PassRecorder->SetVertexBuffer(0, PreviewMeshBuffers.VertexBuffer);
				PassRecorder->SetIndexBuffer(PreviewMeshBuffers.IndexBuffer);
				PassRecorder->DrawIndexed(0, PreviewMeshBuffers.IndexCount);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
		return true;
	}

	void MaterialPreviewRenderer::RenderErrorColor(uint32 InWidth, uint32 InHeight)
	{
		ResizeRenderTargetsIfNeeded(InWidth, InHeight);
		if (!EnsureErrorPipeline())
		{
			return;
		}
		if (!EnsurePreviewMesh())
		{
			return;
		}

		{
			Camera ViewCamera;
			ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
			ViewCamera.AspectRatio = (float)InWidth / (float)InHeight;
			ViewCamera.NearPlane = 0.1f;
			ViewCamera.FarPlane = 10.0f;
			ViewCamera.Yaw = OrbitYaw;
			ViewCamera.Pitch = OrbitPitch;

			const FMatrix44f OrbitRotation = RotationMatrix(ViewCamera.Yaw, ViewCamera.Pitch);
			const Vector4f OrbitPos = OrbitRotation.TransformFVector4(FVector4f(0.0f, 0.0f, -CameraDistance, 1.0f));
			ViewCamera.Position = Vector3f(OrbitPos.X, OrbitPos.Y, OrbitPos.Z);

			ErrorUniformBuffer->GetMember<FMatrix44f>("Transform") = ViewCamera.GetViewProjectionMatrix();
		}

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MsaaRenderTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			PreviewClearColor,
			RenderTarget->GetDefaultView()
		});
		PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
			DepthTarget->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::DontCare,
			1.0f
		};

		auto CmdRecorder = GGpuRhi->BeginRecording("MaterialPreviewError");
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, "MaterialPreviewError");
			{
				PassRecorder->SetRenderPipelineState(ErrorPipeline);
				PassRecorder->SetBindGroups({ ErrorBindGroup });
				PassRecorder->SetVertexBuffer(0, PreviewMeshBuffers.VertexBuffer);
				PassRecorder->SetIndexBuffer(PreviewMeshBuffers.IndexBuffer);
				PassRecorder->DrawIndexed(0, PreviewMeshBuffers.IndexCount);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
	}

	FText MaterialPreviewRenderer::GetErrorReason() const
	{
		TArray<FText> Errors;
		if (!MaterialAsset->VertexShaderAsset)
			Errors.Add(LOCALIZATION("VsNotSpecified"));
		else if (!MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex))
			Errors.Add(LOCALIZATION("VsInvalidShader"));

		if (!MaterialAsset->PixelShaderAsset)
			Errors.Add(LOCALIZATION("PsNotSpecified"));
		else if (!MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel))
			Errors.Add(LOCALIZATION("PsInvalidShader"));

		if (LinkageErrorFunc)
			Errors.Add(LinkageErrorFunc());

		TArray<FString> ErrorStrings;
		for (const auto& E : Errors) ErrorStrings.Add(E.ToString());
		return FText::FromString(FString::Join(ErrorStrings, TEXT("\n")));
	}

	TRefCountPtr<GpuTexture> MaterialPreviewRenderer::RenderThumbnail(const Material* InMaterial, uint32 InSize, MaterialPreviewPrimitive InPreviewPrimitive)
	{
		MaterialPreviewRenderer Renderer(const_cast<Material*>(InMaterial));
		Renderer.SetPreviewPrimitive(InPreviewPrimitive);
		Renderer.SetOrbit(0.0f, 0.0f);
		if (!Renderer.Render(InSize, InSize))
		{
			Renderer.RenderErrorColor(InSize, InSize);
		}
		return Renderer.RenderTarget;
	}

	bool MaterialPreviewRenderer::EnsurePipeline()
	{
		if (Pipeline.IsValid())
		{
			return true;
		}

		if (!MaterialAsset->VertexShaderAsset || !MaterialAsset->PixelShaderAsset)
		{
			return false;
		}

		GpuShader* Vs = MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex);
		GpuShader* Ps = MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel);
		if (!Vs || !Ps)
		{
			return false;
		}

		if (Vs->GetShaderLanguage() != Ps->GetShaderLanguage())
		{
			LinkageErrorFunc = []{ return LOCALIZATION("ShaderLanguageMismatch"); };
			return false;
		}

		// Validate VS output / PS input linkage before creating pipeline
		{
			TArray<GpuShaderStageSemantic> VsOutputs = Vs->GetStageOutputSemantics();
			TArray<GpuShaderStageSemantic> PsInputs = Ps->GetStageInputSemantics();
			TArray<FString> MissingSemantics;
			for (const auto& PsInput : PsInputs)
			{
				if (!PsInput.bRead)
				{
					continue;
				}

				if(Vs->GetShaderLanguage() == GpuShaderLanguage::HLSL)
				{
					const GpuShaderStageSemantic* MatchingVsOutput = VsOutputs.FindByPredicate([&](const GpuShaderStageSemantic& VsOutput) {
						return VsOutput.SemanticName.Equals(PsInput.SemanticName, ESearchCase::IgnoreCase)
							&& VsOutput.SemanticIndex == PsInput.SemanticIndex;
					});
					if (!MatchingVsOutput || !MatchingVsOutput->bWritten)
					{
						FString SemanticStr = PsInput.SemanticIndex > 0
							? FString::Printf(TEXT("%s%d"), *PsInput.SemanticName, PsInput.SemanticIndex)
							: PsInput.SemanticName;
						MissingSemantics.Add(MoveTemp(SemanticStr));
					}
				}
				else
				{
					const GpuShaderStageSemantic* MatchingVsOutput = VsOutputs.FindByPredicate([&](const GpuShaderStageSemantic& VsOutput) {
						return VsOutput.Location == PsInput.Location;
					});
					if (!MatchingVsOutput || !MatchingVsOutput->bWritten)
					{
						FString Desc = FString::Printf(TEXT("(location %d) %s"), PsInput.Location, *PsInput.Name);
						MissingSemantics.Add(MoveTemp(Desc));
					}
				}
			}
			if (MissingSemantics.Num() > 0)
			{
				FString Joined = FString::Join(MissingSemantics, TEXT(", "));
				LinkageErrorFunc = [Joined]{ return FText::Format(LOCALIZATION("SemanticLinkageError"), FText::FromString(Joined)); };
				return false;
			}

			LinkageErrorFunc = nullptr;
		}

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
			if (LinkageErrorFunc)
			{
				return false;
			}
		}

		// Build vertex layout dynamically from Material's VertexInputDefaults
		GpuVertexLayoutDesc VertexLayoutDesc;
		VertexLayoutDesc.ByteStride = sizeof(MeshVertex);
		for (const auto& InputDefault : MaterialAsset->VertexInputDefaults)
		{
			GpuVertexAttributeDesc AttrDesc;
			AttrDesc.Location = InputDefault.Location;
			AttrDesc.SemanticName = InputDefault.SemanticName;
			AttrDesc.SemanticIndex = InputDefault.SemanticIndex;

			switch (InputDefault.Attribute)
			{
			case BuiltInVertexAttribute::BuiltInPosition:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Position);
				AttrDesc.Format = GpuFormat::R32G32B32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInNormal:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Normal);
				AttrDesc.Format = GpuFormat::R32G32B32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInUV0:
				AttrDesc.ByteOffset = offsetof(MeshVertex, UVs[0]);
				AttrDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInUV1:
				AttrDesc.ByteOffset = offsetof(MeshVertex, UVs[1]);
				AttrDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInUV2:
				AttrDesc.ByteOffset = offsetof(MeshVertex, UVs[2]);
				AttrDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInUV3:
				AttrDesc.ByteOffset = offsetof(MeshVertex, UVs[3]);
				AttrDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInColor:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Color);
				AttrDesc.Format = GpuFormat::R32G32B32A32_FLOAT;
				break;
			case BuiltInVertexAttribute::BuiltInTangent:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Tangent);
				AttrDesc.Format = GpuFormat::R32G32B32A32_FLOAT;
				break;
			default:
				break;
			}

			VertexLayoutDesc.Attributes.Add(MoveTemp(AttrDesc));
		}

		TArray<GpuBindGroupLayout*> BindGroupLayoutArray;
		for (auto& [_, BindGroupLayout] : BindGroupLayouts)
		{
			BindGroupLayoutArray.Add(BindGroupLayout.GetReference());
		}

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = {
				{
					.TargetFormat = PreviewColorFormat,
					.BlendEnable = MaterialAsset->BlendEnable,
					.SrcFactor = MaterialAsset->SrcBlendFactor,
					.ColorOp = MaterialAsset->ColorBlendOp,
					.DestFactor = MaterialAsset->DestBlendFactor,
				}
			},
			.BindGroupLayouts = MoveTemp(BindGroupLayoutArray),
			.VertexLayout = { MoveTemp(VertexLayoutDesc) },
			.RasterizerState = {
				.FillMode = MaterialAsset->FillMode,
				.CullMode = MaterialAsset->CullMode,
			},
			.SampleCount = PreviewSampleCount,
			.DepthStencilState = MaterialAsset->DepthTestEnable ?
				TOptional<DepthStencilStateDesc>(DepthStencilStateDesc{
					.DepthFormat = PreviewDepthFormat,
					.DepthCompare = MaterialAsset->DepthCompare,
				}) : TOptional<DepthStencilStateDesc>(),
		};

		try
		{
			Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
		}
		catch (const std::runtime_error& e)
		{
			FString ErrorMsg = ANSI_TO_TCHAR(e.what());
			LinkageErrorFunc = [ErrorMsg]{ return FText::FromString(ErrorMsg); };
		}
		return Pipeline.IsValid();
	}

	bool MaterialPreviewRenderer::EnsureErrorPipeline()
	{
		if (ErrorPipeline.IsValid())
		{
			return true;
		}

		UniformBufferBuilder UbBuilder{ UniformBufferUsage::Persistant };
		UbBuilder.AddMatrix4x4f("Transform");
		ErrorUniformBuffer = UbBuilder.Build();

		ErrorBindGroupLayout = GpuBindGroupLayoutBuilder{0}
			.AddUniformBuffer("ModelUb", UbBuilder, BindingShaderStage::Vertex)
			.Build();

		ErrorBindGroup = GpuBindGroupBuilder{ErrorBindGroupLayout}
			.SetUniformBuffer("ModelUb", ErrorUniformBuffer->GetGpuResource())
			.Build();

		FString UbDecl = ErrorBindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);
		FString VsSource = UbDecl + TEXT(R"(
struct VsOutput { float4 Position : SV_POSITION; };
VsOutput MainVS(float3 Position : POSITION0) {
	VsOutput Output;
	Output.Position = mul(float4(Position, 1.0), Transform);
	return Output;
}
)");
		FString PsSource = TEXT(R"(
float4 MainPS() : SV_Target { return float4(1.0, 0.0, 1.0, 1.0); }
)");

		TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromSource({
			.Name = "MaterialErrorVS",
			.Source = VsSource,
			.Type = ShaderType::Vertex,
			.EntryPoint = "MainVS",
		});
		TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromSource({
			.Name = "MaterialErrorPS",
			.Source = PsSource,
			.Type = ShaderType::Pixel,
			.EntryPoint = "MainPS",
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		if (!ErrorInfo.IsEmpty()) return false;
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		if (!ErrorInfo.IsEmpty()) return false;

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = {
				{ .TargetFormat = PreviewColorFormat }
			},
			.BindGroupLayouts = { ErrorBindGroupLayout },
			.VertexLayout = {
				{
					.ByteStride = sizeof(MeshVertex),
					.Attributes = {
						{
							.Location = 0,
							.SemanticName = "POSITION",
							.Format = GpuFormat::R32G32B32_FLOAT,
							.ByteOffset = offsetof(MeshVertex, Position),
						},
					}
				}
			},
			.RasterizerState = {
				.FillMode = RasterizerFillMode::Solid,
				.CullMode = RasterizerCullMode::Back,
			},
			.SampleCount = PreviewSampleCount,
			.DepthStencilState = DepthStencilStateDesc{
				.DepthFormat = PreviewDepthFormat,
			},
		};

		ErrorPipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
		return ErrorPipeline.IsValid();
	}

	void MaterialPreviewRenderer::BuildBindGroupFromMaterial()
	{
		PreviewUniformBuffers.Empty();
		BindGroupLayouts.Empty();
		BindGroups.Empty();

		// Collect bindings from both shaders
		TArray<GpuShaderLayoutBinding> AllBindings;
		GpuShader* VsForBindGroup = MaterialAsset->VertexShaderAsset ? MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex) : nullptr;
		if (VsForBindGroup)
		{
			AllBindings.Append(VsForBindGroup->GetLayout());
		}
		GpuShader* PsForBindGroup = MaterialAsset->PixelShaderAsset ? MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel) : nullptr;
		if (PsForBindGroup)
		{
			AllBindings.Append(PsForBindGroup->GetLayout());
		}

		TMap<BindingGroupSlot, TArray<const GpuShaderLayoutBinding*>> GroupedBindings;
		for (const auto& Binding : AllBindings)
		{
			GroupedBindings.FindOrAdd(Binding.Group).Add(&Binding);
		}

		// Merge bindings with same slot+type (for UBs, also require same layout): combine stage visibility
		auto UbLayoutsMatch = [](const TArray<GpuShaderUbMemberInfo>& A, const TArray<GpuShaderUbMemberInfo>& B) {
			if (A.Num() != B.Num()) return false;
			for (int32 i = 0; i < A.Num(); ++i)
			{
				if (A[i].Name != B[i].Name || A[i].Offset != B[i].Offset || A[i].Size != B[i].Size || A[i].Type != B[i].Type)
					return false;
			}
			return true;
		};

		auto MakeUbKey = [](const FString& Name, BindingShaderStage Stage) -> FString {
			if (Stage == BindingShaderStage::Vertex) return Name + TEXT("|VS");
			if (Stage == BindingShaderStage::Pixel) return Name + TEXT("|PS");
			return Name;
		};

		TMap<BindingGroupSlot, TArray<GpuShaderLayoutBinding>> MergedGroupedBindings;
		for (const auto& [GroupSlot, Bindings] : GroupedBindings)
		{
			auto& MergedBindings = MergedGroupedBindings.FindOrAdd(GroupSlot);
			for (const auto* Binding : Bindings)
			{
				auto* Existing = MergedBindings.FindByPredicate([&](const GpuShaderLayoutBinding& M) {
					if (M.Slot != Binding->Slot || M.Type != Binding->Type) return false;
					if (M.Type == BindingType::UniformBuffer)
						return UbLayoutsMatch(M.UbMembers, Binding->UbMembers);
					return true;
				});
				if (Existing)
				{
					if (Existing->Name != Binding->Name)
						Existing->Name = Existing->Name + TEXT("/") + Binding->Name;
					Existing->Stage = Existing->Stage | Binding->Stage;
				}
				else
				{
					// For UBs with different layout at same slot, keep separate entries
					MergedBindings.Add(*Binding);
				}
			}
		}

		for (auto& [GroupSlot, MergedBindings] : MergedGroupedBindings)
		{
			GpuBindGroupLayoutBuilder LayoutBuilder{GroupSlot};
			for (const auto& Binding : MergedBindings)
			{
				LayoutBuilder.AddExistingBinding(Binding.Slot, Binding.Type, Binding.Stage);
			}
			BindGroupLayouts.Add(GroupSlot, LayoutBuilder.Build());
		}

		// Build UBs and bind groups per group
		for (auto& [GroupSlot, MergedBindings] : MergedGroupedBindings)
		{
			GpuBindGroupBuilder GroupBuilder{BindGroupLayouts[GroupSlot]};

			for (const auto& Binding : MergedBindings)
			{
				if (Binding.Type == BindingType::UniformBuffer)
				{
					FString UbKey = MakeUbKey(Binding.Name, Binding.Stage);
					if (!PreviewUniformBuffers.Contains(UbKey))
					{
						PreviewUniformBuffers.Add(UbKey, BuildUniformBufferFromReflection(Binding.UbMembers));
					}
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, PreviewUniformBuffers[UbKey]->GetGpuResource(), Binding.Stage);
				}
			}

			// Bind resource defaults
			for (const auto& Binding : MergedBindings)
			{
				if (Binding.Type == BindingType::UniformBuffer) continue;

				const MaterialBindingResourceDefault* ResDefault = nullptr;
				for (const auto& D : MaterialAsset->BindingResourceDefaults)
				{
					if (D.BindingName == Binding.Name)
					{
						ResDefault = &D;
						break;
					}
				}
				if (!ResDefault) continue;

				auto GetTextureView = [](const MaterialBindingResourceDefault& D, BindingType InType) -> GpuResource* {
					if (D.TextureAsset)
					{
						GpuTexture* GpuData = nullptr;
						if (auto* Tex2D = dynamic_cast<Texture2D*>(D.TextureAsset.Get()))
							GpuData = Tex2D->GetGpuData();
						else if (auto* TexCube = dynamic_cast<TextureCube*>(D.TextureAsset.Get()))
							GpuData = TexCube->GetGpuData();
						else if (auto* Tex3D = dynamic_cast<Texture3D*>(D.TextureAsset.Get()))
							GpuData = Tex3D->GetGpuData();
						if (GpuData)
							return GpuData->GetDefaultView();
					}
					// Fallback to global black texture
					if (InType == BindingType::TextureCube || InType == BindingType::CombinedTextureCubeSampler)
						return GpuResourceHelper::GetGlobalBlackCubemapTex()->GetDefaultView();
					if (InType == BindingType::Texture3D || InType == BindingType::CombinedTexture3DSampler)
						return GpuResourceHelper::GetGlobalBlackVolumeTex()->GetDefaultView();
					return GpuResourceHelper::GetGlobalBlackTex()->GetDefaultView();
				};

				auto GetSamplerResource = [](const MaterialBindingResourceDefault& D) -> GpuSampler* {
					GpuSamplerDesc Desc;
					Desc.Filter = D.Filter;
					Desc.AddressU = D.AddressMode;
					Desc.AddressV = D.AddressMode;
					Desc.AddressW = D.AddressMode;
					return GpuResourceHelper::GetSampler(Desc);
				};

				switch (Binding.Type)
				{
				case BindingType::Texture:
				case BindingType::TextureCube:
				case BindingType::Texture3D:
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, GetTextureView(*ResDefault, Binding.Type), Binding.Stage);
					break;
				case BindingType::Sampler:
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, GetSamplerResource(*ResDefault), Binding.Stage);
					break;
				case BindingType::CombinedTextureSampler:
				case BindingType::CombinedTextureCubeSampler:
				case BindingType::CombinedTexture3DSampler:
				{
					GpuResource* TexView = GetTextureView(*ResDefault, Binding.Type);
					GpuSampler* Sampler = GetSamplerResource(*ResDefault);
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, new GpuCombinedTextureSampler(TexView, Sampler), Binding.Stage);
					break;
				}
				default:
					break;
				}
			}

			BindGroups.Add(GroupSlot, GroupBuilder.Build());
		}
	}

	bool MaterialPreviewRenderer::EnsurePreviewMesh()
	{
		if (bPreviewMeshInitialized)
		{
			return true;
		}

		if (PreviewPrimitive == MaterialPreviewPrimitive::Sphere)
		{
			PreviewMeshBuffers = UploadMesh(SphereMesh);
		}
		else if(PreviewPrimitive == MaterialPreviewPrimitive::Cube)
		{
			PreviewMeshBuffers = UploadMesh(CubeMesh);
		}
		else
		{
			PreviewMeshBuffers = UploadMesh(QuadMesh);
		}
		bPreviewMeshInitialized = true;
		return PreviewMeshBuffers.VertexBuffer.IsValid() && PreviewMeshBuffers.IndexBuffer.IsValid();
	}

	void MaterialPreviewRenderer::ResizeRenderTargetsIfNeeded(uint32 InWidth, uint32 InHeight)
	{
		if (RenderTarget.IsValid() && RenderTarget->GetWidth() == InWidth && RenderTarget->GetHeight() == InHeight)
		{
			return;
		}

		GpuTextureDesc TargetDesc{
			.Width = InWidth,
			.Height = InHeight,
			.Format = PreviewColorFormat,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			.ClearValues = PreviewClearColor,
		};
		RenderTarget = GGpuRhi->CreateTexture(TargetDesc);

		GpuTextureDesc MsaaDesc = TargetDesc;
		MsaaDesc.Usage = GpuTextureUsage::RenderTarget;
		MsaaDesc.SampleCount = PreviewSampleCount;
		MsaaRenderTarget = GGpuRhi->CreateTexture(MsaaDesc);

		GpuTextureDesc DepthDesc{
			.Width = InWidth,
			.Height = InHeight,
			.Format = PreviewDepthFormat,
			.Usage = GpuTextureUsage::DepthStencil,
			.SampleCount = PreviewSampleCount,
		};
		DepthTarget = GGpuRhi->CreateTexture(DepthDesc);
	}

	void MaterialPreviewRenderer::UpdatePreviewTransform(uint32 InWidth, uint32 InHeight)
	{
		Camera ViewCamera;
		ViewCamera.VerticalFov = FMath::DegreesToRadians(45.0f);
		ViewCamera.AspectRatio = (float)InWidth / (float)InHeight;
		ViewCamera.NearPlane = 0.1f;
		ViewCamera.FarPlane = 10.0f;
		ViewCamera.Yaw = OrbitYaw;
		ViewCamera.Pitch = OrbitPitch;

		const FMatrix44f OrbitRotation = RotationMatrix(ViewCamera.Yaw, ViewCamera.Pitch);
		const Vector4f OrbitPos = OrbitRotation.TransformFVector4(FVector4f(0.0f, 0.0f, -CameraDistance, 1.0f));
		ViewCamera.Position = Vector3f(OrbitPos.X, OrbitPos.Y, OrbitPos.Z);

		const FMatrix44f ModelMatrix = FMatrix44f::Identity;
		const FMatrix44f ViewMatrix = ViewCamera.GetViewMatrix();
		const FMatrix44f ProjMatrix = ViewCamera.GetProjectionMatrix();
		const FMatrix44f ViewProjMatrix = ViewMatrix * ProjMatrix;
		const FMatrix44f MVPMatrix = ModelMatrix * ViewProjMatrix;

		for (auto& [UbKey, Ub] : PreviewUniformBuffers)
		{
			FString UbBaseName = UbKey;
			BindingShaderStage UbStage = BindingShaderStage::All;
			if (UbKey.EndsWith(TEXT("|VS")))
			{
				UbBaseName = UbKey.LeftChop(3);
				UbStage = BindingShaderStage::Vertex;
			}
			else if (UbKey.EndsWith(TEXT("|PS")))
			{
				UbBaseName = UbKey.LeftChop(3);
				UbStage = BindingShaderStage::Pixel;
			}

			for (const auto& Default : MaterialAsset->BindingMemberDefaults)
			{
				if (Default.BindingName != UbBaseName) continue;
				if (UbStage != BindingShaderStage::All && Default.Stage != UbStage) continue;

				if (IsShaderMatrix4x4Type(Default.Type))
				{
					if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInModel)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = ModelMatrix;
					else if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInView)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = ViewMatrix;
					else if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInProj)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = ProjMatrix;
					else if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInViewProj)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = ViewProjMatrix;
					else if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInMVP)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = MVPMatrix;
				}
				else if (Default.Type == TEXT("float"))
					Ub->GetMember<float>(Default.MemberName) = Default.Values[0];
				else if (Default.Type == TEXT("int") || IsShaderBoolType(Default.Type))
					Ub->GetMember<int32>(Default.MemberName) = Default.IntValues[0];
				else if (Default.Type == TEXT("uint"))
					Ub->GetMember<uint32>(Default.MemberName) = Default.UintValues[0];
				else if (IsShaderVector2Type(Default.Type))
					Ub->GetMember<Vector2f>(Default.MemberName) = *reinterpret_cast<const Vector2f*>(Default.Values);
				else if (IsShaderVector3Type(Default.Type))
					Ub->GetMember<Vector3f>(Default.MemberName) = *reinterpret_cast<const Vector3f*>(Default.Values);
				else if (IsShaderVector4Type(Default.Type))
					Ub->GetMember<Vector4f>(Default.MemberName) = *reinterpret_cast<const Vector4f*>(Default.Values);
				else if (IsShaderIntVector2Type(Default.Type) || IsShaderBoolVector2Type(Default.Type))
					Ub->GetMember<Vector2i>(Default.MemberName) = *reinterpret_cast<const Vector2i*>(Default.IntValues);
				else if (IsShaderIntVector3Type(Default.Type) || IsShaderBoolVector3Type(Default.Type))
					Ub->GetMember<Vector3i>(Default.MemberName) = *reinterpret_cast<const Vector3i*>(Default.IntValues);
				else if (IsShaderIntVector4Type(Default.Type) || IsShaderBoolVector4Type(Default.Type))
					Ub->GetMember<Vector4i>(Default.MemberName) = *reinterpret_cast<const Vector4i*>(Default.IntValues);
				else if (IsShaderUintVector2Type(Default.Type))
					Ub->GetMember<Vector2u>(Default.MemberName) = *reinterpret_cast<const Vector2u*>(Default.UintValues);
				else if (IsShaderUintVector3Type(Default.Type))
					Ub->GetMember<Vector3u>(Default.MemberName) = *reinterpret_cast<const Vector3u*>(Default.UintValues);
				else if (IsShaderUintVector4Type(Default.Type))
					Ub->GetMember<Vector4u>(Default.MemberName) = *reinterpret_cast<const Vector4u*>(Default.UintValues);
			}
		}
	}
}
