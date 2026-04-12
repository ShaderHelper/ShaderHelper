#include "CommonHeader.h"
#include "MetalDevice.h"
#include "MetalArgumentBuffer.h"
#include "MetalTexture.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{
    static bool HasRenderStage(BindingShaderStage InStage)
    {
        return EnumHasAnyFlags(InStage, BindingShaderStage::Vertex | BindingShaderStage::Pixel);
    }

    static MTLRenderStages MapShaderVisibility(BindingShaderStage InStage)
    {
        MTLRenderStages Stages = 0;
        
        if(EnumHasAnyFlags(InStage, BindingShaderStage::Vertex))
        {
            Stages |= MTLRenderStageVertex;
        }
        
        if(EnumHasAnyFlags(InStage, BindingShaderStage::Pixel))
        {
            Stages |= MTLRenderStageFragment;
        }

        return Stages;
    }

    MetalBindGroupLayout::MetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
        : GpuBindGroupLayout(LayoutDesc)
    {
        TArray<MTLArgumentDescriptor*> ArgDescs;

        for(const auto& [Slot, LayoutBindingEntry] : Desc.Layouts)
        {
            if(HasRenderStage(LayoutBindingEntry.Stage))
            {
                RenderStages.Add(Slot, MapShaderVisibility(LayoutBindingEntry.Stage));
            }
            
            MTLArgumentDescriptor* ArgDesc = [MTLArgumentDescriptor argumentDescriptor];
            ArgDesc.index = Slot.SlotNum + GetBindingShift(LayoutBindingEntry.Type);
            MTLArgumentDescriptor* CombinedSamplerArgDesc = nil;
            if(LayoutBindingEntry.Type == BindingType::RWStructuredBuffer || LayoutBindingEntry.Type == BindingType::RWRawBuffer)
            {
                ArgDesc.dataType = MTLDataTypePointer;
                ArgDesc.access = MTLArgumentAccessReadWrite;
                ResourceUsages.Add(Slot, MTLResourceUsageWrite);
            }
            else if(LayoutBindingEntry.Type == BindingType::UniformBuffer || LayoutBindingEntry.Type == BindingType::StructuredBuffer || LayoutBindingEntry.Type == BindingType::RawBuffer)
            {
                ArgDesc.dataType = MTLDataTypePointer;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::CombinedTextureSampler || LayoutBindingEntry.Type == BindingType::CombinedTextureCubeSampler || LayoutBindingEntry.Type == BindingType::CombinedTexture3DSampler)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = LayoutBindingEntry.Type == BindingType::CombinedTextureCubeSampler ? MTLTextureTypeCube :
                    (LayoutBindingEntry.Type == BindingType::CombinedTexture3DSampler ? MTLTextureType3D : MTLTextureType2D);
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);

				CombinedSamplerArgDesc = [MTLArgumentDescriptor argumentDescriptor];
				CombinedSamplerArgDesc.index = Slot.SlotNum + GetBindingShift(LayoutBindingEntry.Type) + 1;
				CombinedSamplerArgDesc.dataType = MTLDataTypeSampler;
				CombinedSamplerArgDesc.access = MTLArgumentAccessReadOnly;
            }
            else if(LayoutBindingEntry.Type == BindingType::Texture)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType2D;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::TextureCube)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureTypeCube;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::Texture3D)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType3D;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::RWTexture)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType2D;
                ArgDesc.access = MTLArgumentAccessReadWrite;
                ResourceUsages.Add(Slot, MTLResourceUsageRead | MTLResourceUsageWrite);
            }
            else if(LayoutBindingEntry.Type == BindingType::RWTexture3D)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType3D;
                ArgDesc.access = MTLArgumentAccessReadWrite;
                ResourceUsages.Add(Slot, MTLResourceUsageRead | MTLResourceUsageWrite);
            }
            else if(LayoutBindingEntry.Type == BindingType::Sampler)
            {
                ArgDesc.dataType = MTLDataTypeSampler;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else
            {
                check(false);
            }
            
            ArgDescs.Add(ArgDesc);
            
            // Add sampler descriptor AFTER the texture descriptor to keep indices sorted
            if(CombinedSamplerArgDesc != nil)
            {
                ArgDescs.Add(CombinedSamplerArgDesc);
            }
        }
        
        if(ArgDescs.Num() > 0)
        {
            ArgumentEncoder = NS::TransferPtr(GDevice->newArgumentEncoder(
                (NS::Array*)[NSArray arrayWithObjects:ArgDescs.GetData() count:ArgDescs.Num()]
            ));
        }

    }

    MetalBindGroup::MetalBindGroup(const GpuBindGroupDesc& InDesc)
        : GpuBindGroup(InDesc)
    {
        GMtlDeferredReleaseManager.AddResource(this);
        
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(InDesc.Layout.GetReference());
        MTL::ArgumentEncoder* ArgumentEncoder = BindGroupLayout->GetArgumentEncoder();

        if(ArgumentEncoder != nullptr)
        {
			ArgumentBuffer = CreateMetalBuffer({(uint32)ArgumentEncoder->encodedLength(), GpuBufferUsage::Upload});
            ArgumentEncoder->setArgumentBuffer(ArgumentBuffer->GetResource(), 0);
        }
        
		auto Layouts = BindGroupLayout->GetDesc().Layouts;
        for(const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
        {
            if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(ResourceBindingEntry.Resource.GetReference());
                if(ArgumentEncoder != nullptr)
                {
                    ArgumentEncoder->setBuffer(Buffer->GetResource(), 0, Slot.SlotNum + GetBindingShift(Slot.Type));
                }
                BindGroupResources.Add(Slot, Buffer->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Texture)
            {
                MetalTexture* Tex = static_cast<MetalTexture*>(ResourceBindingEntry.Resource.GetReference());
                MetalTextureView* TexView = Tex->GetMtlDefaultView();
				if(ArgumentEncoder != nullptr)
                {
					ArgumentEncoder->setTexture(TexView->GetResource(), Slot.SlotNum + GetBindingShift(Slot.Type));
				}
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::TextureView)
            {
                MetalTextureView* TexView = static_cast<MetalTextureView*>(ResourceBindingEntry.Resource.GetReference());
				if(ArgumentEncoder != nullptr)
                {
					ArgumentEncoder->setTexture(TexView->GetResource(), Slot.SlotNum + GetBindingShift(Slot.Type));
				}
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Sampler)
            {
                MetalSampler* Sampler = static_cast<MetalSampler*>(ResourceBindingEntry.Resource.GetReference());
				if(ArgumentEncoder != nullptr)
                {
                    ArgumentEncoder->setSamplerState(Sampler->GetResource(), Slot.SlotNum + GetBindingShift(Slot.Type));
                }
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::CombinedTextureSampler)
            {
                GpuCombinedTextureSampler* Combined = static_cast<GpuCombinedTextureSampler*>(ResourceBindingEntry.Resource.GetReference());
                MetalTexture* Tex = static_cast<MetalTexture*>(Combined->GetTexture());
                MetalTextureView* TexView = Tex->GetMtlDefaultView();
                MetalSampler* Sampler = static_cast<MetalSampler*>(Combined->GetSampler());
				if(ArgumentEncoder != nullptr)
                {
					ArgumentEncoder->setTexture(TexView->GetResource(), Slot.SlotNum + GetBindingShift(Slot.Type));
					ArgumentEncoder->setSamplerState(Sampler->GetResource(), Slot.SlotNum + GetBindingShift(Slot.Type) + 1);
				}
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else
            {
                check(false);
            }
        }
    }

    void MetalBindGroup::Apply(MTL::RenderCommandEncoder* Encoder)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        auto Layouts = BindGroupLayout->GetDesc().Layouts;
        for(auto [Slot ,MtlResource] : BindGroupResources)
        {
            if(HasRenderStage(Layouts[Slot].Stage))
            {
                Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot), BindGroupLayout->GetRenderStages(Slot));
            }
        }
        
        if(ArgumentBuffer)
        {
            Encoder->setVertexBuffer(ArgumentBuffer->GetResource(), 0, BindGroupLayout->GetGroupNumber());
            Encoder->setFragmentBuffer(ArgumentBuffer->GetResource(), 0, BindGroupLayout->GetGroupNumber());
        }
        
    }

    void MetalBindGroup::Apply(MTL::ComputeCommandEncoder* Encoder)
    {
         MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        auto Layouts = BindGroupLayout->GetDesc().Layouts;
        for(auto [Slot ,MtlResource] : BindGroupResources)
        {
            if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
            {
                Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot));
            }
        }

        if(ArgumentBuffer)
        {
			Encoder->setBuffer(ArgumentBuffer->GetResource(), 0, BindGroupLayout->GetGroupNumber());
        }
    }

    TRefCountPtr<MetalBindGroupLayout> CreateMetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
    {
        return new MetalBindGroupLayout(LayoutDesc);
    }

    TRefCountPtr<MetalBindGroup> CreateMetalBindGroup(const GpuBindGroupDesc& Desc)
    {
        return new MetalBindGroup(Desc);
    }
}


