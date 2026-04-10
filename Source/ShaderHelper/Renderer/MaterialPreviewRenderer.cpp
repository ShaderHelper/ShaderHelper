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

using namespace FW;

namespace SH
{
	namespace
	{
		constexpr uint32 PreviewSampleCount = 4;
		constexpr GpuFormat PreviewColorFormat = GpuFormat::B8G8R8A8_UNORM;
		constexpr GpuFormat PreviewDepthFormat = GpuFormat::D32_FLOAT;
		const Vector4f PreviewClearColor(0.08f, 0.08f, 0.08f, 1.0f);

		void AddUbMemberToBuilder(UniformBufferBuilder& Builder, const GpuShaderUbMemberInfo& Member)
		{
			if (IsShaderMatrix4x4Type(Member.Type))             Builder.AddMatrix4x4f(Member.Name);
			else if (IsShaderVector4Type(Member.Type))          Builder.AddVector4f(Member.Name);
			else if (IsShaderVector3Type(Member.Type))          Builder.AddVector3f(Member.Name);
			else if (IsShaderVector2Type(Member.Type))          Builder.AddVector2f(Member.Name);
			else if (Member.Type == TEXT("float"))             Builder.AddFloat(Member.Name);
			else if (Member.Type == TEXT("int") || IsShaderBoolType(Member.Type))  Builder.AddInt(Member.Name);
			else if (Member.Type == TEXT("uint"))              Builder.AddUint(Member.Name);
			else if (IsShaderIntVector4Type(Member.Type) || IsShaderBoolVector4Type(Member.Type))  Builder.AddVector4i(Member.Name);
			else if (IsShaderIntVector3Type(Member.Type) || IsShaderBoolVector3Type(Member.Type))  Builder.AddVector3i(Member.Name);
			else if (IsShaderIntVector2Type(Member.Type) || IsShaderBoolVector2Type(Member.Type))  Builder.AddVector2i(Member.Name);
			else if (IsShaderUintVector4Type(Member.Type))     Builder.AddVector4u(Member.Name);
			else if (IsShaderUintVector3Type(Member.Type))     Builder.AddVector3u(Member.Name);
			else if (IsShaderUintVector2Type(Member.Type))     Builder.AddVector2u(Member.Name);
		}
	}

	MaterialPreviewRenderer::MaterialPreviewRenderer(Material* InMaterial)
		: MaterialAsset(InMaterial)
	{
		check(MaterialAsset);
	}

	void MaterialPreviewRenderer::SetMaterial(Material* InMaterial)
	{
		MaterialAsset = InMaterial;
		Pipeline = nullptr;
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		PreviewUniformBuffers.Empty();
		bPreviewMeshInitialized = false;
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

	FString MaterialPreviewRenderer::GetErrorReason() const
	{
		TArray<FString> Errors;
		if (!MaterialAsset->VertexShaderAsset)
			Errors.Add(LOCALIZATION("VsNotSpecified").ToString());
		else if (!MaterialAsset->VertexShaderAsset->Shader || !MaterialAsset->VertexShaderAsset->Shader->IsCompiled())
			Errors.Add(LOCALIZATION("VsCompileFailed").ToString());

		if (!MaterialAsset->PixelShaderAsset)
			Errors.Add(LOCALIZATION("PsNotSpecified").ToString());
		else if (!MaterialAsset->PixelShaderAsset->Shader || !MaterialAsset->PixelShaderAsset->Shader->IsCompiled())
			Errors.Add(LOCALIZATION("PsCompileFailed").ToString());

		return FString::Join(Errors, TEXT("\n"));
	}

	TRefCountPtr<GpuTexture> MaterialPreviewRenderer::RenderThumbnail(const Material* InMaterial, uint32 InSize, MaterialPreviewPrimitive InPreviewPrimitive)
	{
		MaterialPreviewRenderer Renderer(const_cast<Material*>(InMaterial));
		Renderer.SetPreviewPrimitive(InPreviewPrimitive);
		Renderer.SetOrbit(PI, 0.0f);
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

		GpuShader* Vs = MaterialAsset->VertexShaderAsset->Shader;
		GpuShader* Ps = MaterialAsset->PixelShaderAsset->Shader;
		if (!Vs || !Vs->IsCompiled() || !Ps || !Ps->IsCompiled())
		{
			return false;
		}

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
		}

		// Build vertex layout dynamically from Material's VertexInputDefaults
		GpuVertexLayoutDesc VertexLayoutDesc;
		VertexLayoutDesc.ByteStride = sizeof(MeshVertex);
		for (const auto& InputDefault : MaterialAsset->VertexInputDefaults)
		{
			if (InputDefault.Attribute == BuiltInVertexAttribute::None)
			{
				continue;
			}

			GpuVertexAttributeDesc AttrDesc;
			AttrDesc.Location = InputDefault.Location;
			AttrDesc.SemanticName = InputDefault.SemanticName;
			AttrDesc.SemanticIndex = InputDefault.SemanticIndex;
			AttrDesc.Format = ShaderTypeToGpuFormat(InputDefault.Type);

			switch (InputDefault.Attribute)
			{
			case BuiltInVertexAttribute::BuiltInPosition:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Position);
				break;
			case BuiltInVertexAttribute::BuiltInNormal:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Normal);
				break;
			case BuiltInVertexAttribute::BuiltInUV:
				AttrDesc.ByteOffset = offsetof(MeshVertex, UV);
				break;
			case BuiltInVertexAttribute::BuiltInColor:
				AttrDesc.ByteOffset = offsetof(MeshVertex, Color);
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

		Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
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
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
		});
		TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromSource({
			.Name = "MaterialErrorPS",
			.Source = PsSource,
			.Type = ShaderType::PixelShader,
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
		if (MaterialAsset->VertexShaderAsset && MaterialAsset->VertexShaderAsset->Shader && MaterialAsset->VertexShaderAsset->Shader->IsCompiled())
		{
			AllBindings.Append(MaterialAsset->VertexShaderAsset->Shader->GetLayout());
		}
		if (MaterialAsset->PixelShaderAsset && MaterialAsset->PixelShaderAsset->Shader && MaterialAsset->PixelShaderAsset->Shader->IsCompiled())
		{
			AllBindings.Append(MaterialAsset->PixelShaderAsset->Shader->GetLayout());
		}

		TMap<BindingGroupSlot, TArray<const GpuShaderLayoutBinding*>> GroupedBindings;
		for (const auto& Binding : AllBindings)
		{
			GroupedBindings.FindOrAdd(Binding.Group).Add(&Binding);
		}

		for (auto& [GroupSlot, Bindings] : GroupedBindings)
		{
			GpuBindGroupLayoutBuilder LayoutBuilder{GroupSlot};
			TSet<BindingSlot> AddedSlots;
			for (const auto* Binding : Bindings)
			{
				BindingSlot BSlot{Binding->Slot, Binding->Type};
				if (!AddedSlots.Contains(BSlot))
				{
					AddedSlots.Add(BSlot);
					LayoutBuilder.AddExistingBinding(Binding->Slot, Binding->Type);
				}
			}
			BindGroupLayouts.Add(GroupSlot, LayoutBuilder.Build());
		}

		// Build UBs and bind groups per group
		for (auto& [GroupSlot, Bindings] : GroupedBindings)
		{
			GpuBindGroupBuilder GroupBuilder{BindGroupLayouts[GroupSlot]};
			TSet<BindingSlot> BuiltSlots;

			for (const auto* Binding : Bindings)
			{
				BindingSlot BSlot{Binding->Slot, Binding->Type};
				if (Binding->Type == BindingType::UniformBuffer && !BuiltSlots.Contains(BSlot))
				{
					BuiltSlots.Add(BSlot);
					UniformBufferBuilder UbBuilder{ UniformBufferUsage::Persistant };
					for (const auto& Member : Binding->UbMembers)
					{
						AddUbMemberToBuilder(UbBuilder, Member);
					}
					PreviewUniformBuffers.Add(Binding->Name, UbBuilder.Build());
					GroupBuilder.SetExistingBinding(Binding->Slot, Binding->Type, PreviewUniformBuffers[Binding->Name]->GetGpuResource());
				}
			}

			// Bind resource defaults
			for (const auto* Binding : Bindings)
			{
				BindingSlot BSlot{Binding->Slot, Binding->Type};
				if (BuiltSlots.Contains(BSlot)) continue;
				BuiltSlots.Add(BSlot);

				const MaterialBindingResourceDefault* ResDefault = nullptr;
				for (const auto& D : MaterialAsset->BindingResourceDefaults)
				{
					if (D.BindingName == Binding->Name)
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

				switch (Binding->Type)
				{
				case BindingType::Texture:
				case BindingType::TextureCube:
				case BindingType::Texture3D:
					GroupBuilder.SetExistingBinding(Binding->Slot, Binding->Type, GetTextureView(*ResDefault, Binding->Type));
					break;
				case BindingType::Sampler:
					GroupBuilder.SetExistingBinding(Binding->Slot, Binding->Type, GetSamplerResource(*ResDefault));
					break;
				case BindingType::CombinedTextureSampler:
				case BindingType::CombinedTextureCubeSampler:
				case BindingType::CombinedTexture3DSampler:
				{
					GpuResource* TexView = GetTextureView(*ResDefault, Binding->Type);
					GpuSampler* Sampler = GetSamplerResource(*ResDefault);
					GroupBuilder.SetExistingBinding(Binding->Slot, Binding->Type, new GpuCombinedTextureSampler(TexView, Sampler));
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
		const FMatrix44f ViewProjMatrix = ViewCamera.GetViewProjectionMatrix();

		for (auto& [UbName, Ub] : PreviewUniformBuffers)
		{
			for (const auto& Default : MaterialAsset->BindingMemberDefaults)
			{
				if (Default.BindingName != UbName) continue;

				if (IsShaderMatrix4x4Type(Default.Type))
				{
					if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInModel)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = ModelMatrix;
					else if (Default.MatrixValue == BuiltInMatrix4x4Value::BuiltInViewProj)
						Ub->GetMember<FMatrix44f>(Default.MemberName) = ViewProjMatrix;
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
