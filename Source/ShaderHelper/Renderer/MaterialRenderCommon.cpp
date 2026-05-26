#include "CommonHeader.h"
#include "Renderer/MaterialRenderCommon.h"
#include "AssetObject/Render/ShaderOverrideHelper.h"
#include "RenderResource/Mesh.h"
#include "RenderResource/Camera.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture3D.h"
#include "GpuApi/GpuResourceHelper.h"
#include "RenderResource/PrintBuffer.h"
#include "Common/Util/Math.h"

#include <stdexcept>
using namespace FW;

namespace SH
{
	namespace
	{
		FString BuildMaterialErrorPixelShaderSource(int32 ColorTargetCount)
		{
			if (ColorTargetCount <= 0)
			{
				return TEXT("void MainPS() {}\n");
			}

			FString Source = TEXT("struct PsOutput {\n");
			for (int32 TargetIndex = 0; TargetIndex < ColorTargetCount; ++TargetIndex)
			{
				Source += FString::Printf(TEXT("\tfloat4 Color%d : SV_Target%d;\n"), TargetIndex, TargetIndex);
			}
			Source += TEXT("};\n");
			Source += TEXT("PsOutput MainPS() {\n");
			Source += TEXT("\tPsOutput Output;\n");
			for (int32 TargetIndex = 0; TargetIndex < ColorTargetCount; ++TargetIndex)
			{
				Source += FString::Printf(TEXT("\tOutput.Color%d = float4(1.0, 0.0, 1.0, 1.0);\n"), TargetIndex);
			}
			Source += TEXT("\treturn Output;\n");
			Source += TEXT("}\n");
			return Source;
		}

		bool EnsureMaterialErrorBindGroup(MaterialErrorRenderResources& Resources)
		{
			if (Resources.BindGroupLayout.IsValid() && Resources.BindGroup.IsValid() && Resources.UniformBuffer)
			{
				return true;
			}

			UniformBufferBuilder UbBuilder{ UniformBufferUsage::Persistant };
			UbBuilder.AddMatrix4x4f("Transform");
			Resources.UniformBuffer = UbBuilder.Build();

			Resources.BindGroupLayout = GpuBindGroupLayoutBuilder{0}
				.AddUniformBuffer("ModelUb", UbBuilder, BindingShaderStage::Vertex)
				.Build();

			Resources.BindGroup = GpuBindGroupBuilder{Resources.BindGroupLayout}
				.SetUniformBuffer("ModelUb", Resources.UniformBuffer->GetGpuResource())
				.Build();

			return Resources.BindGroupLayout.IsValid() && Resources.BindGroup.IsValid() && Resources.UniformBuffer;
		}

		bool UniformBufferLayoutsMatch(const TArray<GpuShaderUbMemberInfo>& A, const TArray<GpuShaderUbMemberInfo>& B)
		{
			if (A.Num() != B.Num()) return false;
			for (int32 i = 0; i < A.Num(); ++i)
			{
				if (A[i].Name != B[i].Name || A[i].Offset != B[i].Offset || A[i].Size != B[i].Size || A[i].Type != B[i].Type)
				{
					return false;
				}
			}
			return true;
		}

		template<typename ValueType>
		ValueType ReadUniformValue(const uint8* SourceBytes)
		{
			ValueType Value{};
			FMemory::Memcpy(&Value, SourceBytes, sizeof(ValueType));
			return Value;
		}

		GpuResource* GetTextureView(ShaderResourceBindingState* ResourceState, BindingType InType, GpuTexture* OverrideTexture, Vector2f ViewportSize)
		{
			if (OverrideTexture) return OverrideTexture->GetDefaultView();
			GpuTexture* Tex = ResolveOverrideTexture(InType, nullptr, ResourceState, ViewportSize);
			return Tex->GetDefaultView();
		}

		MaterialBindingResourceDefault* FindResourceDefault(TArray<MaterialBindingResourceDefault>& ResourceDefaults, const FString& BindingName, BindingShaderStage Stage)
		{
			for (auto& ResourceDefault : ResourceDefaults)
			{
				if (ResourceDefault.BindingName == BindingName && ResourceDefault.Stage == Stage)
				{
					return &ResourceDefault;
				}
			}
			return nullptr;
		}

		bool IsTextureSamplerStateBinding(BindingType BindingTypeValue)
		{
			switch (BindingTypeValue)
			{
			case BindingType::Texture:
			case BindingType::TextureCube:
			case BindingType::Texture3D:
			case BindingType::Sampler:
			case BindingType::CombinedTextureSampler:
			case BindingType::CombinedTextureCubeSampler:
			case BindingType::CombinedTexture3DSampler:
			case BindingType::RWTexture:
			case BindingType::RWTexture3D:
				return true;
			default:
				return false;
			}
		}

		void ParseUniformBufferKey(const FString& InKey, FString& OutBindingName, BindingShaderStage& OutStage)
		{
			OutBindingName = InKey;
			OutStage = BindingShaderStage::All;
			if (InKey.EndsWith(TEXT("|VS")))
			{
				OutBindingName = InKey.LeftChop(3);
				OutStage = BindingShaderStage::Vertex;
			}
			else if (InKey.EndsWith(TEXT("|PS")))
			{
				OutBindingName = InKey.LeftChop(3);
				OutStage = BindingShaderStage::Pixel;
			}
		}

		void ApplyMaterialUniformMember(UniformBuffer& InOutUniformBuffer, const MaterialBindingMemberDefault& MemberDefault, const MaterialUniformBufferUpdateOptions& Options)
		{
			const uint8* SourceBytes = Options.UniformOverrideBytesResolver ? Options.UniformOverrideBytesResolver(MemberDefault) : nullptr;

			if (IsShaderMatrix4x4Type(MemberDefault.Type))
			{
				if (SourceBytes)
				{
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = ReadUniformValue<FMatrix44f>(SourceBytes);
					return;
				}

				if (MemberDefault.ValueSource == MaterialBindingValueSource::Custom)
				{
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = ReadUniformValue<FMatrix44f>(reinterpret_cast<const uint8*>(MemberDefault.Values));
					return;
				}

				switch (MemberDefault.MatrixValue)
				{
				case BuiltInMatrix4x4Value::Model:
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = Options.ModelMatrix;
					break;
				case BuiltInMatrix4x4Value::View:
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = Options.ViewMatrix;
					break;
				case BuiltInMatrix4x4Value::Proj:
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = Options.ProjMatrix;
					break;
				case BuiltInMatrix4x4Value::ViewProj:
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = Options.ViewProjMatrix;
					break;
				case BuiltInMatrix4x4Value::MVP:
					InOutUniformBuffer.GetMember<FMatrix44f>(MemberDefault.MemberName) = Options.MVPMatrix;
					break;
				}
				return;
			}

			if (IsShaderFloatType(MemberDefault.Type))
			{
				if (SourceBytes)
				{
					InOutUniformBuffer.GetMember<float>(MemberDefault.MemberName) = ReadUniformValue<float>(SourceBytes);
				}
				else if (MemberDefault.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					InOutUniformBuffer.GetMember<float>(MemberDefault.MemberName) = Options.Time;
				}
				else
				{
					InOutUniformBuffer.GetMember<float>(MemberDefault.MemberName) = ReadUniformValue<float>(reinterpret_cast<const uint8*>(MemberDefault.Values));
				}
				return;
			}

			if (IsShaderVector2Type(MemberDefault.Type))
			{
				if (SourceBytes)
				{
					InOutUniformBuffer.GetMember<Vector2f>(MemberDefault.MemberName) = ReadUniformValue<Vector2f>(SourceBytes);
				}
				else if (MemberDefault.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					switch (MemberDefault.Vector2Value)
					{
					case BuiltInVector2Value::ViewportSize:
						InOutUniformBuffer.GetMember<Vector2f>(MemberDefault.MemberName) = Options.ViewportSize;
						break;
					case BuiltInVector2Value::MousePos:
						InOutUniformBuffer.GetMember<Vector2f>(MemberDefault.MemberName) = Options.MousePos;
						break;
					}
				}
				else
				{
					InOutUniformBuffer.GetMember<Vector2f>(MemberDefault.MemberName) = ReadUniformValue<Vector2f>(reinterpret_cast<const uint8*>(MemberDefault.Values));
				}
				return;
			}

			if (IsShaderVector3Type(MemberDefault.Type))
			{
				if (SourceBytes)
				{
					InOutUniformBuffer.GetMember<Vector3f>(MemberDefault.MemberName) = ReadUniformValue<Vector3f>(SourceBytes);
				}
				else if (MemberDefault.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					switch (MemberDefault.Vector3Value)
					{
					case BuiltInVector3Value::CameraPos:
						InOutUniformBuffer.GetMember<Vector3f>(MemberDefault.MemberName) = Options.CameraPos;
						break;
					case BuiltInVector3Value::CameraDir:
						InOutUniformBuffer.GetMember<Vector3f>(MemberDefault.MemberName) = Options.CameraDir;
						break;
					}
				}
				else
				{
					InOutUniformBuffer.GetMember<Vector3f>(MemberDefault.MemberName) = ReadUniformValue<Vector3f>(reinterpret_cast<const uint8*>(MemberDefault.Values));
				}
				return;
			}

			if (!SourceBytes)
			{
				SourceBytes = reinterpret_cast<const uint8*>(MemberDefault.Values);
			}

			if (IsShaderIntType(MemberDefault.Type) || IsShaderBoolType(MemberDefault.Type))
				InOutUniformBuffer.GetMember<int32>(MemberDefault.MemberName) = ReadUniformValue<int32>(SourceBytes);
			else if (IsShaderUintType(MemberDefault.Type))
				InOutUniformBuffer.GetMember<uint32>(MemberDefault.MemberName) = ReadUniformValue<uint32>(SourceBytes);
			else if (IsShaderVector4Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector4f>(MemberDefault.MemberName) = ReadUniformValue<Vector4f>(SourceBytes);
			else if (IsShaderIntVector2Type(MemberDefault.Type) || IsShaderBoolVector2Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector2i>(MemberDefault.MemberName) = ReadUniformValue<Vector2i>(SourceBytes);
			else if (IsShaderIntVector3Type(MemberDefault.Type) || IsShaderBoolVector3Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector3i>(MemberDefault.MemberName) = ReadUniformValue<Vector3i>(SourceBytes);
			else if (IsShaderIntVector4Type(MemberDefault.Type) || IsShaderBoolVector4Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector4i>(MemberDefault.MemberName) = ReadUniformValue<Vector4i>(SourceBytes);
			else if (IsShaderUintVector2Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector2u>(MemberDefault.MemberName) = ReadUniformValue<Vector2u>(SourceBytes);
			else if (IsShaderUintVector3Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector3u>(MemberDefault.MemberName) = ReadUniformValue<Vector3u>(SourceBytes);
			else if (IsShaderUintVector4Type(MemberDefault.Type))
				InOutUniformBuffer.GetMember<Vector4u>(MemberDefault.MemberName) = ReadUniformValue<Vector4u>(SourceBytes);
		}
	}

	TOptional<GpuVertexLayoutDesc> BuildMaterialMeshVertexLayout(const Material& InMaterial)
	{
		if (InMaterial.VertexInputDefaults.IsEmpty())
		{
			return {};
		}

		GpuVertexLayoutDesc VertexLayoutDesc;
		VertexLayoutDesc.ByteStride = sizeof(MeshVertex);
		for (const auto& InputDefault : InMaterial.VertexInputDefaults)
		{
			GpuVertexAttributeDesc AttributeDesc;
			AttributeDesc.Location = InputDefault.Location;
			AttributeDesc.SemanticName = InputDefault.SemanticName;
			AttributeDesc.SemanticIndex = InputDefault.SemanticIndex;

			switch (InputDefault.Attribute)
			{
			case BuiltInVertexAttribute::Position:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, Position);
				AttributeDesc.Format = GpuFormat::R32G32B32_FLOAT;
				break;
			case BuiltInVertexAttribute::Normal:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, Normal);
				AttributeDesc.Format = GpuFormat::R32G32B32_FLOAT;
				break;
			case BuiltInVertexAttribute::UV0:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, UVs[0]);
				AttributeDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::UV1:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, UVs[1]);
				AttributeDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::UV2:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, UVs[2]);
				AttributeDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::UV3:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, UVs[3]);
				AttributeDesc.Format = GpuFormat::R32G32_FLOAT;
				break;
			case BuiltInVertexAttribute::Color:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, Color);
				AttributeDesc.Format = GpuFormat::R32G32B32A32_FLOAT;
				break;
			case BuiltInVertexAttribute::Tangent:
				AttributeDesc.ByteOffset = offsetof(MeshVertex, Tangent);
				AttributeDesc.Format = GpuFormat::R32G32B32A32_FLOAT;
				break;
			default:
				break;
			}

			VertexLayoutDesc.Attributes.Add(MoveTemp(AttributeDesc));
		}
		return VertexLayoutDesc;
	}

	TUniquePtr<UniformBuffer> BuildMaterialUniformBufferFromReflection(const TArray<GpuShaderUbMemberInfo>& Members)
	{
		UniformBufferMetaData MetaData;
		uint32 MaxEnd = 0;
		for (const auto& Member : Members)
		{
			FString HlslType, GlslType;
			if (IsShaderMatrix4x4Type(Member.Type))                                                         { HlslType = TEXT("float4x4"); GlslType = TEXT("mat4"); }
			else if (IsShaderVector4Type(Member.Type))                                                      { HlslType = TEXT("float4"); GlslType = TEXT("vec4"); }
			else if (IsShaderVector3Type(Member.Type))                                                      { HlslType = TEXT("float3"); GlslType = TEXT("vec3"); }
			else if (IsShaderVector2Type(Member.Type))                                                      { HlslType = TEXT("float2"); GlslType = TEXT("vec2"); }
			else if (IsShaderFloatType(Member.Type))                                                         { HlslType = TEXT("float"); GlslType = TEXT("float"); }
			else if (IsShaderIntType(Member.Type) || IsShaderBoolType(Member.Type))                          { HlslType = TEXT("int"); GlslType = TEXT("int"); }
			else if (IsShaderUintType(Member.Type))                                                          { HlslType = TEXT("uint"); GlslType = TEXT("uint"); }
			else if (IsShaderIntVector4Type(Member.Type) || IsShaderBoolVector4Type(Member.Type))            { HlslType = TEXT("int4"); GlslType = TEXT("ivec4"); }
			else if (IsShaderIntVector3Type(Member.Type) || IsShaderBoolVector3Type(Member.Type))            { HlslType = TEXT("int3"); GlslType = TEXT("ivec3"); }
			else if (IsShaderIntVector2Type(Member.Type) || IsShaderBoolVector2Type(Member.Type))            { HlslType = TEXT("int2"); GlslType = TEXT("ivec2"); }
			else if (IsShaderUintVector4Type(Member.Type))                                                  { HlslType = TEXT("uint4"); GlslType = TEXT("uvec4"); }
			else if (IsShaderUintVector3Type(Member.Type))                                                  { HlslType = TEXT("uint3"); GlslType = TEXT("uvec3"); }
			else if (IsShaderUintVector2Type(Member.Type))                                                  { HlslType = TEXT("uint2"); GlslType = TEXT("uvec2"); }
			else                                                                                            { HlslType = Member.Type; GlslType = Member.Type; }
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

	FString MakeMaterialUniformBufferKey(const FString& Name, BindingShaderStage Stage)
	{
		if (Stage == BindingShaderStage::Vertex) return Name + TEXT("|VS");
		if (Stage == BindingShaderStage::Pixel) return Name + TEXT("|PS");
		return Name;
	}

	void BuildMaterialBindGroups(
		Material& InMaterial,
		TMap<int32, TRefCountPtr<GpuBindGroupLayout>>& OutBindGroupLayouts,
		TMap<int32, TRefCountPtr<GpuBindGroup>>& OutBindGroups,
		TMap<FString, TUniquePtr<UniformBuffer>>& OutUniformBuffers,
		const MaterialBindGroupBuildOptions& Options)
	{
		if (Options.bRebuildUniformBuffers)
		{
			OutUniformBuffers.Empty();
		}
		if (Options.bRebuildLayouts)
		{
			OutBindGroupLayouts.Empty();
		}
		OutBindGroups.Empty();
		TArray<MaterialBindingResourceDefault>& BindingResourceDefaults = Options.BindingResourceDefaults
			? *Options.BindingResourceDefaults
			: InMaterial.BindingResourceDefaults;

		TArray<GpuShaderLayoutBinding> AllBindings;
		GpuShader* Vs = InMaterial.VertexShaderAsset ? InMaterial.VertexShaderAsset->GetCompiledShader(ShaderType::Vertex) : nullptr;
		GpuShader* Ps = InMaterial.PixelShaderAsset ? InMaterial.PixelShaderAsset->GetCompiledShader(ShaderType::Pixel) : nullptr;
		if (Vs) AllBindings.Append(Vs->GetLayout());
		if (Ps) AllBindings.Append(Ps->GetLayout());

		TMap<BindingGroupSlot, TArray<const GpuShaderLayoutBinding*>> GroupedBindings;
		for (const auto& Binding : AllBindings)
		{
			GroupedBindings.FindOrAdd(Binding.Group).Add(&Binding);
		}

		TMap<BindingGroupSlot, TArray<GpuShaderLayoutBinding>> MergedGroupedBindings;
		for (const auto& [GroupSlot, Bindings] : GroupedBindings)
		{
			auto& MergedBindings = MergedGroupedBindings.FindOrAdd(GroupSlot);
			for (const auto* Binding : Bindings)
			{
				auto* Existing = MergedBindings.FindByPredicate([&](const GpuShaderLayoutBinding& ExistingBinding) {
					if (ExistingBinding.Slot != Binding->Slot || ExistingBinding.Type != Binding->Type) return false;
					if (ExistingBinding.Type == BindingType::UniformBuffer) return UniformBufferLayoutsMatch(ExistingBinding.UbMembers, Binding->UbMembers);
					return true;
				});
				if (Existing)
				{
					if (Existing->Name != Binding->Name) Existing->Name = Existing->Name + TEXT("/") + Binding->Name;
					Existing->Stage = Existing->Stage | Binding->Stage;
					if (Existing->StructuredStride == 0)
					{
						Existing->StructuredStride = Binding->StructuredStride;
					}
				}
				else
				{
					MergedBindings.Add(*Binding);
				}
			}
		}

		for (auto& [GroupSlot, MergedBindings] : MergedGroupedBindings)
		{
			TRefCountPtr<GpuBindGroupLayout>& BindGroupLayout = OutBindGroupLayouts.FindOrAdd(GroupSlot);
			if (Options.bRebuildLayouts || !BindGroupLayout.IsValid())
			{
				GpuBindGroupLayoutBuilder LayoutBuilder{GroupSlot};
				for (const auto& Binding : MergedBindings)
				{
					LayoutBuilder.AddExistingBinding(Binding.Slot, Binding.Type, Binding.Stage);
				}
				BindGroupLayout = LayoutBuilder.Build();
			}
		}

		for (auto& [GroupSlot, MergedBindings] : MergedGroupedBindings)
		{
			GpuBindGroupBuilder GroupBuilder{OutBindGroupLayouts[GroupSlot]};

			for (const auto& Binding : MergedBindings)
			{
				if (Binding.Type != BindingType::UniformBuffer) continue;

				FString UniformBufferKey = MakeMaterialUniformBufferKey(Binding.Name, Binding.Stage);
				if (!OutUniformBuffers.Contains(UniformBufferKey))
				{
					OutUniformBuffers.Add(UniformBufferKey, BuildMaterialUniformBufferFromReflection(Binding.UbMembers));
				}
				if (OutUniformBuffers[UniformBufferKey])
				{
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, OutUniformBuffers[UniformBufferKey]->GetGpuResource(), Binding.Stage);
				}
			}

			for (const auto& Binding : MergedBindings)
			{
				if (Binding.Type == BindingType::UniformBuffer) continue;

				if (Binding.Name == TEXT("GPrivate_Printer") && Binding.Type == BindingType::RWRawBuffer)
				{
					GpuBuffer* PrinterBuffer = Options.PrinterBuffer ? Options.PrinterBuffer : TSingleton<PrintBuffer>::Get().GetResource();
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, PrinterBuffer, Binding.Stage);
					continue;
				}

				const bool bUsesTextureSamplerState = IsTextureSamplerStateBinding(Binding.Type);
				GpuTexture* OverrideTexture = (bUsesTextureSamplerState && Binding.Type != BindingType::Sampler && Options.TextureOverrideResolver) ? Options.TextureOverrideResolver(Binding) : nullptr;
				MaterialBindingResourceDefault* ResourceDefault = FindResourceDefault(BindingResourceDefaults, Binding.Name, Binding.Stage);
				check(ResourceDefault);
				ShaderResourceBindingState* ResourceState = ResourceDefault;

				switch (Binding.Type)
				{
				case BindingType::StructuredBuffer:
				case BindingType::RWStructuredBuffer:
				case BindingType::RawBuffer:
				case BindingType::RWRawBuffer:
				case BindingType::TypedBuffer:
				case BindingType::RWTypedBuffer:
				{
					ResolveDefaultBuffer(ResourceDefault->BufferByteSize, Binding.StructuredStride, ResourceDefault->BufferFormat, Binding.Type, ResourceDefault->Buffer);
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, ResourceDefault->Buffer, Binding.Stage);
					break;
				}
				case BindingType::Texture:
				case BindingType::TextureCube:
				case BindingType::Texture3D:
				case BindingType::RWTexture:
				case BindingType::RWTexture3D:
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, GetTextureView(ResourceState, Binding.Type, OverrideTexture, Options.DefaultResourceViewportSize), Binding.Stage);
					break;
				case BindingType::Sampler:
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, ResolveResourceSampler(ResourceState), Binding.Stage);
					break;
				case BindingType::CombinedTextureSampler:
				case BindingType::CombinedTextureCubeSampler:
				case BindingType::CombinedTexture3DSampler:
				{
					GpuResource* TextureView = GetTextureView(ResourceState, Binding.Type, OverrideTexture, Options.DefaultResourceViewportSize);
					GpuSampler* Sampler = ResolveResourceSampler(ResourceState);
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, new GpuCombinedTextureSampler(TextureView, Sampler), Binding.Stage);
					break;
				}
				default:
					break;
				}
			}

			OutBindGroups.Add(GroupSlot, GroupBuilder.Build());
		}
	}

	void UpdateMaterialUniformBuffers(
		const Material& InMaterial,
		TMap<FString, TUniquePtr<UniformBuffer>>& InOutUniformBuffers,
		const MaterialUniformBufferUpdateOptions& Options)
	{
		for (auto& [UniformBufferKey, UniformBufferPtr] : InOutUniformBuffers)
		{
			if (!UniformBufferPtr) continue;

			FString BindingName;
			BindingShaderStage Stage;
			ParseUniformBufferKey(UniformBufferKey, BindingName, Stage);

			for (const auto& MemberDefault : InMaterial.BindingMemberDefaults)
			{
				if (MemberDefault.BindingName != BindingName) continue;
				if (Stage != BindingShaderStage::All && MemberDefault.Stage != Stage) continue;

				ApplyMaterialUniformMember(*UniformBufferPtr, MemberDefault, Options);
			}
		}
	}

	bool EnsureMaterialErrorRenderResources(
		MaterialErrorRenderResources& InOutResources,
		const TArray<GpuFormat>& ColorFormats,
		GpuFormat DepthFormat,
		uint32 SampleCount)
	{
		if (InOutResources.Pipeline.IsValid()
			&& InOutResources.ColorFormats == ColorFormats
			&& InOutResources.DepthFormat == DepthFormat
			&& InOutResources.SampleCount == SampleCount)
		{
			return true;
		}

		InOutResources.Pipeline.SafeRelease();
		InOutResources.ColorFormats.Empty();
		InOutResources.DepthFormat = GpuFormat::NUM;
		InOutResources.SampleCount = 0;

		if (!EnsureMaterialErrorBindGroup(InOutResources))
		{
			return false;
		}

		FString VsSource = InOutResources.BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);
		VsSource += TEXT(R"(
struct VsOutput { float4 Position : SV_POSITION; };
VsOutput MainVS(float3 Position : POSITION0) {
	VsOutput Output;
	Output.Position = mul(float4(Position, 1.0), Transform);
	return Output;
}
)");
		FString PsSource = BuildMaterialErrorPixelShaderSource(ColorFormats.Num());

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

		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;
		for (GpuFormat ColorFormat : ColorFormats)
		{
			Targets.Add(PipelineTargetDesc{ .TargetFormat = ColorFormat });
		}

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = MoveTemp(Targets),
			.BindGroupLayouts = { InOutResources.BindGroupLayout },
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
			.SampleCount = SampleCount,
			.DepthStencilState = DepthFormat != GpuFormat::NUM
				? TOptional<DepthStencilStateDesc>(DepthStencilStateDesc{ .DepthFormat = DepthFormat })
				: TOptional<DepthStencilStateDesc>(),
		};

		try
		{
			InOutResources.Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);
		}
		catch (const std::runtime_error&)
		{
			return false;
		}

		if (!InOutResources.Pipeline.IsValid())
		{
			return false;
		}

		InOutResources.ColorFormats = ColorFormats;
		InOutResources.DepthFormat = DepthFormat;
		InOutResources.SampleCount = SampleCount;
		return true;
	}

	void DrawMaterialErrorMesh(
		GpuRenderPassRecorder* Recorder,
		MaterialErrorRenderResources& Resources,
		const MeshBuffers& MeshBuffers,
		const FMatrix44f& Transform)
	{
		Resources.UniformBuffer->GetMember<FMatrix44f>("Transform") = Transform;

		Recorder->SetRenderPipelineState(Resources.Pipeline);
		Recorder->SetBindGroups({ Resources.BindGroup.GetReference() });
		Recorder->SetVertexBuffer(0, MeshBuffers.VertexBuffer);
		Recorder->SetIndexBuffer(MeshBuffers.IndexBuffer);
		Recorder->DrawIndexed(0, MeshBuffers.IndexCount);
	}

	GpuTexture* ResolveTextureAssetGpu(AssetObject* TextureAsset)
	{
		if (auto* Texture = DynamicCast<Texture2D>(TextureAsset)) return Texture->GetGpuData();
		if (auto* Texture = DynamicCast<TextureCube>(TextureAsset)) return Texture->GetGpuData();
		if (auto* Texture = DynamicCast<Texture3D>(TextureAsset)) return Texture->GetGpuData();
		return nullptr;
	}

	Vector3f GetCameraForwardDir(const Camera& InCamera)
	{
		const Vector4f Forward = InCamera.GetWorldRotationMatrix().TransformFVector4(FVector4f(0.0f, 0.0f, 1.0f, 0.0f));
		return FVector3f(Forward.X, Forward.Y, Forward.Z).GetSafeNormal();
	}

	MaterialUniformBufferUpdateOptions MakeMaterialUniformOptions(
		const FMatrix44f& ModelMatrix,
		const FMatrix44f& ViewMatrix,
		const FMatrix44f& ProjMatrix,
		Vector2f ViewportSize,
		Vector2f MousePos,
		Vector3f CameraPos,
		Vector3f CameraDir,
		float Time)
	{
		MaterialUniformBufferUpdateOptions Options;
		Options.ModelMatrix = ModelMatrix;
		Options.ViewMatrix = ViewMatrix;
		Options.ProjMatrix = ProjMatrix;
		Options.ViewProjMatrix = ViewMatrix * ProjMatrix;
		Options.MVPMatrix = ModelMatrix * Options.ViewProjMatrix;
		Options.ViewportSize = ViewportSize;
		Options.MousePos = MousePos;
		Options.CameraPos = CameraPos;
		Options.CameraDir = CameraDir;
		Options.Time = Time;
		return Options;
	}

	bool BuildMaterialPipeline(
		const Material& InMaterial,
		const MaterialPipelineBuildOptions& Options,
		MaterialPipelineBuildResult& OutResult)
	{
		OutResult = {};

		if (!InMaterial.VertexShaderAsset)
		{
			OutResult.Error = LOCALIZATION("VsNotSpecified");
			return false;
		}
		if (!InMaterial.PixelShaderAsset)
		{
			OutResult.Error = LOCALIZATION("PsNotSpecified");
			return false;
		}

		GpuShader* Vs = InMaterial.VertexShaderAsset->GetCompiledShader(ShaderType::Vertex);
		GpuShader* Ps = InMaterial.PixelShaderAsset->GetCompiledShader(ShaderType::Pixel);
		if (!Vs)
		{
			OutResult.Error = LOCALIZATION("VsInvalidShader");
			return false;
		}
		if (!Ps)
		{
			OutResult.Error = LOCALIZATION("PsInvalidShader");
			return false;
		}
		if (Vs->GetShaderLanguage() != Ps->GetShaderLanguage())
		{
			OutResult.Error = LOCALIZATION("ShaderLanguageMismatch");
			return false;
		}

		// VS output / PS input linkage
		{
			TArray<GpuShaderStageSemantic> VsOutputs = Vs->GetStageOutputSemantics();
			TArray<GpuShaderStageSemantic> PsInputs = Ps->GetStageInputSemantics();
			TArray<FString> MissingSemantics;
			const bool bHlsl = Vs->GetShaderLanguage() == GpuShaderLanguage::HLSL;
			for (const auto& PsInput : PsInputs)
			{
				if (bHlsl)
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
						MissingSemantics.Add(FString::Printf(TEXT("(location %d) %s"), PsInput.Location, *PsInput.Name));
					}
					else if (!MatchingVsOutput->Type.IsEmpty() && !PsInput.Type.IsEmpty()
						&& !MatchingVsOutput->Type.Equals(PsInput.Type, ESearchCase::IgnoreCase))
					{
						MissingSemantics.Add(FString::Printf(TEXT("(location %d) %s (type mismatch: VS=%s, PS=%s)"),
							PsInput.Location, *PsInput.Name, *MatchingVsOutput->Type, *PsInput.Type));
					}
				}
			}
			if (MissingSemantics.Num() > 0)
			{
				FString Joined = FString::Join(MissingSemantics, TEXT(", "));
				OutResult.Error = FText::Format(LOCALIZATION("SemanticLinkageError"), FText::FromString(Joined));
				return false;
			}
		}

		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;
		for (GpuFormat ColorFormat : Options.ColorFormats)
		{
			Targets.Add(PipelineTargetDesc{
				.TargetFormat = ColorFormat,
				.BlendEnable = InMaterial.BlendEnable,
				.SrcFactor = InMaterial.SrcBlendFactor,
				.ColorOp = InMaterial.ColorBlendOp,
				.DestFactor = InMaterial.DestBlendFactor,
			});
		}

		TArray<GpuVertexLayoutDesc> VertexLayouts;
		if (TOptional<GpuVertexLayoutDesc> VertexLayout = BuildMaterialMeshVertexLayout(InMaterial))
		{
			VertexLayouts.Add(MoveTemp(*VertexLayout));
		}

		OutResult.Desc = GpuRenderPipelineStateDesc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = Targets,
			.BindGroupLayouts = Options.BindGroupLayouts,
			.VertexLayout = MoveTemp(VertexLayouts),
			.RasterizerState = { .FillMode = InMaterial.FillMode, .CullMode = InMaterial.CullMode },
			.Primitive = InMaterial.Primitive,
			.SampleCount = Options.SampleCount,
			.DepthStencilState = (Options.DepthFormat != GpuFormat::NUM && InMaterial.DepthTestEnable)
				? TOptional<DepthStencilStateDesc>(DepthStencilStateDesc{ .DepthFormat = Options.DepthFormat, .DepthCompare = InMaterial.DepthCompare })
				: TOptional<DepthStencilStateDesc>(),
		};

		try
		{
			OutResult.Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(OutResult.Desc);
		}
		catch (const std::runtime_error& e)
		{
			OutResult.Error = FText::FromString(ANSI_TO_TCHAR(e.what()));
			return false;
		}
		return OutResult.Pipeline.IsValid();
	}
}
