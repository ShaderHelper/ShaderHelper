#include "CommonHeader.h"
#include "MeshRenderObject.h"
#include "MeshPassNode.h"
#include "App/App.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/GpuRhi.h"
#include "Renderer/MaterialRenderResources.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include <Widgets/Input/SComboButton.h>
#include <stdexcept>

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<MeshRenderObject>("MeshRenderObject")
		.BaseClass<ShObject>()
		.Data<&MeshRenderObject::MeshSceneObjectRef, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("MeshSceneObject"))
		.Data<&MeshRenderObject::MaterialAsset, MetaInfo::Property>(LOCALIZATION("Material"))
	)

	MeshRenderObject::MeshRenderObject()
	{
		ObjectName = FText::FromString(TEXT("RenderObject"));
	}

	MeshRenderObject::~MeshRenderObject()
	{
		UnbindMaterialDelegates();
	}

	void MeshRenderObject::BindMaterialDelegates()
	{
		Material* CurrentMaterial = MaterialAsset.Get();
		if (BoundMaterial != CurrentMaterial)
		{
			UnbindMaterialDelegates();
		}

		if (CurrentMaterial && !MaterialChangedHandle.IsValid())
		{
			BoundMaterial = CurrentMaterial;
			MaterialChangedHandle = CurrentMaterial->OnMaterialChanged.AddLambda([this] { OnMaterialChanged(); });
		}
	}

	void MeshRenderObject::UnbindMaterialDelegates()
	{
		if (BoundMaterial && MaterialChangedHandle.IsValid())
		{
			BoundMaterial->OnMaterialChanged.Remove(MaterialChangedHandle);
			MaterialChangedHandle.Reset();
		}
		BoundMaterial = nullptr;
	}

	void MeshRenderObject::OnMaterialChanged()
	{
		// Prune overrides whose (BindingName, MemberName) no longer exist in Material.
		if (MaterialAsset)
		{
			for (int32 i = OverrideSlots.Num() - 1; i >= 0; --i)
			{
				const MaterialOverrideSlot& Slot = OverrideSlots[i];
				bool bExists = false;
				if (Slot.bIsResource)
				{
					for (const auto& R : MaterialAsset->BindingResourceDefaults)
					{
						if (R.BindingName == Slot.BindingName) { bExists = true; break; }
					}
				}
				else
				{
					for (const auto& M : MaterialAsset->BindingMemberDefaults)
					{
						if (M.BindingName == Slot.BindingName && M.MemberName == Slot.MemberName) { bExists = true; break; }
					}
				}
				if (!bExists)
				{
					RemoveOverride(i);
				}
			}
		}
		InvalidateRenderResources();
	}

	void MeshRenderObject::InvalidateRenderResources()
	{
		Pipeline.SafeRelease();
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		UniformBuffers.Empty();
		bDrawMaterialError = false;
	}

	void MeshRenderObject::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << MeshSceneObjectRef;
		Ar << MaterialAsset;
		Ar << OverrideSlots;

		SerializePolymorphicObjectArray(Ar, OverridePins, this);
	}

	void MeshRenderObject::PostLoad()
	{
		ShObject::PostLoad();
		BindMaterialDelegates();
	}

	TArray<TSharedRef<PropertyData>> MeshRenderObject::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();

		auto OverrideCat = MakeShared<PropertyCategory>(this, FText::FromString(TEXT("Overrides")));
		OverrideCat->SetAddMenuWidget(
			SNew(SComboButton)
			.ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("+")))]
			.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget> {
				FMenuBuilder MenuBuilder(true, nullptr);
				if (MaterialAsset)
				{
					Material* Mat = MaterialAsset.Get();
					for (const auto& MemberDefault : Mat->BindingMemberDefaults)
					{
						bool Used = OverrideSlots.ContainsByPredicate([&](const MaterialOverrideSlot& Slot) {
							return !Slot.bIsResource && Slot.BindingName == MemberDefault.BindingName && Slot.MemberName == MemberDefault.MemberName && Slot.Stage == MemberDefault.Stage;
						});
						if (Used)
						{
							continue;
						}

						FString Label = MemberDefault.BindingName + TEXT(".") + MemberDefault.MemberName;
						MenuBuilder.AddMenuEntry(FText::FromString(Label), FText::GetEmpty(), FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, MemberDefault] {
								AddOverride(MemberDefault.BindingName, MemberDefault.MemberName, MemberDefault.Type, MemberDefault.Stage, false);
								static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
							}))
						);
					}

					for (const auto& ResourceDefault : Mat->BindingResourceDefaults)
					{
						if (ResourceDefault.BindingType != BindingType::Texture && ResourceDefault.BindingType != BindingType::TextureCube
							&& ResourceDefault.BindingType != BindingType::Texture3D
							&& ResourceDefault.BindingType != BindingType::CombinedTextureSampler
							&& ResourceDefault.BindingType != BindingType::CombinedTextureCubeSampler
							&& ResourceDefault.BindingType != BindingType::CombinedTexture3DSampler)
						{
							continue;
						}

						bool Used = OverrideSlots.ContainsByPredicate([&](const MaterialOverrideSlot& Slot) {
							return Slot.bIsResource && Slot.BindingName == ResourceDefault.BindingName;
						});
						if (Used)
						{
							continue;
						}

						FString TypeStr = (ResourceDefault.BindingType == BindingType::TextureCube || ResourceDefault.BindingType == BindingType::CombinedTextureCubeSampler) ? TEXT("TextureCube")
							: (ResourceDefault.BindingType == BindingType::Texture3D || ResourceDefault.BindingType == BindingType::CombinedTexture3DSampler) ? TEXT("Texture3D")
							: TEXT("Texture2D");
						MenuBuilder.AddMenuEntry(FText::FromString(ResourceDefault.BindingName + TEXT(" (") + TypeStr + TEXT(")")), FText::GetEmpty(), FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ResourceDefault, TypeStr] {
								AddOverride(ResourceDefault.BindingName, TEXT(""), TypeStr, ResourceDefault.Stage, true);
								if (auto* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter()))
								{
									OwnerNode->RefreshNodeWidget();
								}
								static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
							}))
						);
					}
				}

				return MenuBuilder.MakeWidget();
			})
		);

		for (int32 Index = 0; Index < OverrideSlots.Num(); ++Index)
		{
			FString Label = OverrideSlots[Index].BindingName
				+ (OverrideSlots[Index].MemberName.IsEmpty() ? TEXT("") : (TEXT(".") + OverrideSlots[Index].MemberName));
			auto Entry = MakeShared<PropertyItemBase>(this, FText::FromString(Label));
			int32 EntryIndex = Index;
			Entry->SetOnDelete([this, EntryIndex] {
				RemoveOverride(EntryIndex);
				if (auto* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter()))
				{
					OwnerNode->RefreshNodeWidget();
				}
				static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
			});
			OverrideCat->AddChild(Entry);
		}
		Result.Add(OverrideCat);

		return Result;
	}

	void MeshRenderObject::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		if (InProperty && InProperty->GetDisplayName().EqualTo(LOCALIZATION("MeshSceneObject")))
		{
			if (MeshSceneObjectRef.IsValid() && MeshSceneObjectRef.Get())
			{
				ObjectName = MeshSceneObjectRef->ObjectName;
			}
		}
		if (InProperty && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Material")))
		{
			BindMaterialDelegates();
			InvalidateRenderResources();
		}
		if (auto* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter()))
		{
			OwnerNode->RefreshNodeWidget();
		}
	}

	ObjectPtr<GraphPin> MeshRenderObject::CreateOverridePin(ShObject* Outer, const FString& Type, bool bIsResource)
	{
		// Only resource overrides use pins; scalar types use inline value slot.
		// For resources, choose pin type based on Type string.
		if (bIsResource)
		{
			if (Type.Contains(TEXT("Cube")))
			{
				return NewShObject<GpuCubemapPin>(Outer);
			}
			if (Type.Contains(TEXT("3D")))
			{
				return NewShObject<GpuTexture3DPin>(Outer);
			}
			return NewShObject<GpuTexturePin>(Outer);
		}
		return {};
	}

	void MeshRenderObject::AddOverride(const FString& BindingName, const FString& MemberName, const FString& Type, BindingShaderStage Stage, bool bIsResource)
	{
		MaterialOverrideSlot Slot;
		Slot.BindingName = BindingName;
		Slot.MemberName = MemberName;
		Slot.Type = Type;
		Slot.Stage = Stage;
		Slot.bIsResource = bIsResource;

		if (bIsResource)
		{
			auto Pin = CreateOverridePin(this, Type, true);
			if (Pin)
			{
				Pin->Direction = PinDirection::Input;
				Pin->ObjectName = FText::FromString(BindingName);
				Slot.PinGuid = Pin->GetGuid();
				OverridePins.Add(MoveTemp(Pin));
			}
		}
		OverrideSlots.Add(MoveTemp(Slot));
		if (auto* OuterMost = GetOuterMost()) OuterMost->MarkDirty();
	}

	void MeshRenderObject::RemoveOverride(int32 SlotIndex)
	{
		if (!OverrideSlots.IsValidIndex(SlotIndex))
		{
			return;
		}
		MaterialOverrideSlot Slot = OverrideSlots[SlotIndex];
		if (Slot.bIsResource && Slot.PinGuid.IsValid())
		{
			OverridePins.RemoveAll([&](const ObjectPtr<GraphPin>& P) {
				if (P->GetGuid() == Slot.PinGuid)
				{
					// break any incoming link
					if (P->SourcePin.IsValid())
					{
						GraphPin* SrcPin = P->GetSourcePin();
						if (SrcPin)
						{
							GraphNode* SrcNode = static_cast<GraphNode*>(SrcPin->GetOuter());
							SrcNode->OutPinToInPin.Remove(SrcPin, P.Get());
						}
						P->SourcePin.Reset();
						P->Refuse();
					}
					return true;
				}
				return false;
			});
		}
		OverrideSlots.RemoveAt(SlotIndex);
		if (auto* OuterMost = GetOuterMost()) OuterMost->MarkDirty();
	}

	GraphPin* MeshRenderObject::FindOverridePin(const FString& BindingName, const FString& MemberName) const
	{
		for (int32 i = 0; i < OverrideSlots.Num(); ++i)
		{
			const MaterialOverrideSlot& S = OverrideSlots[i];
			if (S.BindingName == BindingName && S.MemberName == MemberName && S.bIsResource)
			{
				for (const auto& P : OverridePins)
				{
					if (P->GetGuid() == S.PinGuid) return P.Get();
				}
			}
		}
		return nullptr;
	}

	void MeshRenderObject::BuildBindGroupFromMaterial(bool bRebuildLayouts, bool bRebuildUniformBuffers)
	{
		if (!MaterialAsset) return;

		MaterialBindGroupBuildOptions Options;
		Options.bRebuildLayouts = bRebuildLayouts;
		Options.bRebuildUniformBuffers = bRebuildUniformBuffers;
		Options.TextureOverrideResolver = [this](const GpuShaderLayoutBinding& Binding) -> GpuTexture* {
				if (GraphPin* OverridePin = FindOverridePin(Binding.Name, TEXT("")))
				{
					if (auto* TexturePin = DynamicCast<GpuTexturePin>(OverridePin)) return TexturePin->GetValue();
					if (auto* CubemapPin = DynamicCast<GpuCubemapPin>(OverridePin)) return CubemapPin->GetValue();
					if (auto* Texture3DPin = DynamicCast<GpuTexture3DPin>(OverridePin)) return Texture3DPin->GetValue();
				}
				return nullptr;
			};

		BuildMaterialBindGroups(*MaterialAsset, BindGroupLayouts, BindGroups, UniformBuffers, Options);
	}

	bool MeshRenderObject::BuildPipeline(const TArray<GpuFormat>& ColorFormats, GpuFormat DepthFormat, uint32 SampleCount)
	{
		if (!MaterialAsset || !MaterialAsset->VertexShaderAsset || !MaterialAsset->PixelShaderAsset) return false;

		GpuShader* Vs = MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex);
		GpuShader* Ps = MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel);
		if (!Vs || !Ps) return false;
		if (Vs->GetShaderLanguage() != Ps->GetShaderLanguage()) return false;

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
			if (BindGroupLayouts.IsEmpty()) return false;
		}

		GpuVertexLayoutDesc VertexLayoutDesc = BuildMaterialMeshVertexLayout(*MaterialAsset);

		TArray<GpuBindGroupLayout*> LayoutArray;
		for (auto& [_, L] : BindGroupLayouts) LayoutArray.Add(L.GetReference());

		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;
		for (GpuFormat F : ColorFormats)
		{
			Targets.Add(PipelineTargetDesc{
				.TargetFormat = F,
				.BlendEnable = MaterialAsset->BlendEnable,
				.SrcFactor = MaterialAsset->SrcBlendFactor,
				.ColorOp = MaterialAsset->ColorBlendOp,
				.DestFactor = MaterialAsset->DestBlendFactor,
			});
		}

		GpuRenderPipelineStateDesc Desc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = MoveTemp(Targets),
			.BindGroupLayouts = MoveTemp(LayoutArray),
			.VertexLayout = { MoveTemp(VertexLayoutDesc) },
			.RasterizerState = { .FillMode = MaterialAsset->FillMode, .CullMode = MaterialAsset->CullMode },
			.SampleCount = SampleCount,
			.DepthStencilState = (DepthFormat != GpuFormat::NUM && MaterialAsset->DepthTestEnable)
				? TOptional<DepthStencilStateDesc>(DepthStencilStateDesc{ .DepthFormat = DepthFormat, .DepthCompare = MaterialAsset->DepthCompare })
				: TOptional<DepthStencilStateDesc>(),
		};

		try { Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(Desc); }
		catch (const std::runtime_error&) { return false; }
		return Pipeline.IsValid();
	}

	bool MeshRenderObject::EnsureRenderResources(const TArray<GpuFormat>& ColorFormats, GpuFormat DepthFormat, uint32 SampleCount)
	{
		BindMaterialDelegates();
		bDrawMaterialError = !BuildPipeline(ColorFormats, DepthFormat, SampleCount);
		if (bDrawMaterialError)
		{
			return EnsureMaterialErrorRenderResources(ErrorResources, ColorFormats, DepthFormat, SampleCount);
		}
		return true;
	}

	void MeshRenderObject::Draw(GpuRenderPassRecorder* Recorder, const Camera& InCamera, const FMatrix44f& ModelMatrix)
	{
		if (!MeshSceneObjectRef.IsValid())
		{
			return;
		}
		MeshSceneObject* MSO = MeshSceneObjectRef.Get();
		if (!MSO || !MSO->ModelAsset) return;
		Model* ModelAsset = MSO->ModelAsset.Get();
		if (!ModelAsset) return;

		const FMatrix44f ViewMat = InCamera.GetViewMatrix();
		const FMatrix44f ProjMat = InCamera.GetProjectionMatrix();
		const FMatrix44f ViewProjMat = ViewMat * ProjMat;
		const FMatrix44f MVPMat = ModelMatrix * ViewProjMat;

		if (bDrawMaterialError)
		{
			const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
			for (const MeshBuffers& MeshBuffer : GpuMeshes)
			{
				DrawMaterialErrorMesh(Recorder, ErrorResources, MeshBuffer, MVPMat);
			}
			return;
		}

		if (!MaterialAsset || !Pipeline.IsValid())
		{
			return;
		}

		bool bHasResourceOverride = false;
		for (const auto& Slot : OverrideSlots)
		{
			if (Slot.bIsResource)
			{
				bHasResourceOverride = true;
				break;
			}
		}
		if (bHasResourceOverride)
		{
			BuildBindGroupFromMaterial(false, false);
		}

		MaterialUniformBufferUpdateOptions UniformOptions;
		UniformOptions.ModelMatrix = ModelMatrix;
		UniformOptions.ViewMatrix = ViewMat;
		UniformOptions.ProjMatrix = ProjMat;
		UniformOptions.ViewProjMatrix = ViewProjMat;
		UniformOptions.MVPMatrix = MVPMat;
		UniformOptions.ScalarOverrideResolver = [this](const MaterialBindingMemberDefault& MemberDefault) -> const uint8* {
			const MaterialOverrideSlot* Override = OverrideSlots.FindByPredicate([&](const MaterialOverrideSlot& Slot) {
				return !Slot.bIsResource && Slot.BindingName == MemberDefault.BindingName && Slot.MemberName == MemberDefault.MemberName && Slot.Stage == MemberDefault.Stage;
			});
			return (Override && Override->ScalarBytes.Num() > 0) ? Override->ScalarBytes.GetData() : nullptr;
		};
		UpdateMaterialUniformBuffers(*MaterialAsset, UniformBuffers, UniformOptions);

		TArray<GpuBindGroup*> BGArray;
		for (auto& [_, BG] : BindGroups) BGArray.Add(BG.GetReference());

		const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
		for (const MeshBuffers& MB : GpuMeshes)
		{
			if (!MB.VertexBuffer || !MB.IndexBuffer) continue;
			Recorder->SetRenderPipelineState(Pipeline);
			Recorder->SetBindGroups(BGArray);
			Recorder->SetVertexBuffer(0, MB.VertexBuffer);
			Recorder->SetIndexBuffer(MB.IndexBuffer);
			Recorder->DrawIndexed(0, MB.IndexCount);
		}
	}
}
