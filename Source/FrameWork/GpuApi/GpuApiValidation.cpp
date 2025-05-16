#include "CommonHeader.h"
#include "GpuApiValidation.h"
#include "magic_enum.hpp"

namespace FW
{

	bool ValidateSetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		auto ValidateBindGroupNumber = [](GpuBindGroup* BindGroup, BindingGroupSlot ExpectedSlot)
		{
			if (BindGroup)
			{
				BindingGroupSlot GroupNumber = BindGroup->GetLayout()->GetGroupNumber();
				if (GroupNumber != ExpectedSlot) {
					SH_LOG(LogRhiValidation, Error, TEXT("SetBindGroups Error(Mismatched BindingGroupSlot) - BindGroup%d : (%d), Expected : (%d)"), ExpectedSlot, GroupNumber, ExpectedSlot);
					return false;
				}
			}
			return true;
		};
		return ValidateBindGroupNumber(BindGroup0, 0) && ValidateBindGroupNumber(BindGroup1, 1) && ValidateBindGroupNumber(BindGroup2, 2) && ValidateBindGroupNumber(BindGroup3, 3);
	}

	static bool ValidateBindGroupResource(BindingSlot Slot, const ResourceBinding& InResourceBinding, BindingType LayoutEntryType)
	{
		GpuResource* GroupEntryResource = InResourceBinding.Resource;
		GpuResourceType GroupEntryType = GroupEntryResource->GetType();
		if (LayoutEntryType == BindingType::UniformBuffer)
		{
			if (GroupEntryType != GpuResourceType::Buffer)
			{
				SH_LOG(LogRhiValidation, Error, TEXT("CreateBindGroup Error(Mismatched GpuResourceType)"
					" - Slot%d : (%s), Expected : (Buffer)"), Slot, ANSI_TO_TCHAR(magic_enum::enum_name(GroupEntryType).data()));
				return false;
			}
			else
			{
				GpuBufferUsage BufferUsage = static_cast<GpuBuffer*>(GroupEntryResource)->GetUsage();
				if (!EnumHasAllFlags(BufferUsage, GpuBufferUsage::Uniform))
				{
					SH_LOG(LogRhiValidation, Error, TEXT("CreateBindGroup Error(Mismatched GpuBufferUsage)"
						" - Slot%d : (%s), Expected : (PersistentUniform or TemporaryUniform)"), Slot, ANSI_TO_TCHAR(magic_enum::enum_name(BufferUsage).data()));
					return false;
				}
			}
		}
		return true;
	}

	bool ValidateCreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc)
	{
		const GpuBindGroupLayoutDesc& LayoutDesc = InBindGroupDesc.Layout->GetDesc();
		bool Valid = true;
		for (const auto& [Slot, LayoutBindingEntry] : LayoutDesc.Layouts)
		{
			if (auto* ResourceBindingEntry = InBindGroupDesc.Resources.Find(Slot))
			{
				Valid &= ValidateBindGroupResource(Slot, *ResourceBindingEntry, LayoutBindingEntry.Type);
			}
			else
			{
				SH_LOG(LogRhiValidation, Error, TEXT("CreateBindGroup Error(Missing BindingSlot) - Not find the slot : (%d)  Type: (%s)"), 
					Slot, ANSI_TO_TCHAR(magic_enum::enum_name(LayoutBindingEntry.Type).data()));
				Valid = false;
			}
		}
		return Valid;
	}

    bool ValidateCreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
    {
        TArray<BindingSlot> Slots;
        for(const auto& [Slot, _] : InBindGroupLayoutDesc.Layouts)
        {
            if(Slots.Contains(Slot))
            {
                SH_LOG(LogRhiValidation, Error, TEXT("CreateBindGroupLayout Error(Duplicated BindingSlot) -  slot detected: (%d)"), Slot);
                return false;
            }
            Slots.Add(Slot);
        }
        return true;
    }

	bool ValidateCreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		auto ValidateBindGroupNumber = [](GpuBindGroupLayout* BindGroupLayout, BindingGroupSlot ExpectedSlot)
		{
			if (BindGroupLayout)
			{
				BindingGroupSlot GroupNumber = BindGroupLayout->GetGroupNumber();
				if (GroupNumber != ExpectedSlot) {
					SH_LOG(LogRhiValidation, Error, TEXT("CreateRenderPipelineState Error(Mismatched BindingGroupSlot) - BindGroupLayout%d : (%d), Expected : (%d)"), ExpectedSlot, GroupNumber, ExpectedSlot);
					return false;
				}
			}
			return true;
		};
		return ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout0, 0) && ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout1, 1)
			&& ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout2, 2) && ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout3, 3);
	}

	bool ValidateCreateBuffer(uint32 ByteSize, GpuBufferUsage Usage, GpuResourceState InitState)
	{
        if(ByteSize <= 0)
        {
            //[MTLDebugDevice newBufferWithLength:options:]:642: failed assertion `Buffer Validation Cannot create buffer of zero length`.
            SH_LOG(LogRhiValidation, Error, TEXT("CreateBuffer Error(Incorrect buffer size:%d)"), ByteSize);
            return false;
        }
        
		if (Usage == GpuBufferUsage::Staging || Usage == GpuBufferUsage::Static || Usage == GpuBufferUsage::Dynamic)
		{
			if (InitState == GpuResourceState::Unknown)
			{
				SH_LOG(LogRhiValidation, Error, TEXT("CreateBuffer Error(InitState can not be Unknown) for GpuBufferUsage(%s)"), ANSI_TO_TCHAR(magic_enum::enum_name(Usage).data()));
				return false;
			}
		}
		else if (EnumHasAllFlags(Usage, GpuBufferUsage::Uniform))
		{
			if (InitState != GpuResourceState::Unknown)
			{
				SH_LOG(LogRhiValidation, Error, TEXT("CreateBuffer Error(InitState must be Unknown) for GpuBufferUsage(%s)"), ANSI_TO_TCHAR(magic_enum::enum_name(Usage).data()));
				return false;
			}
		}
		return ValidateGpuResourceState(InitState);
	}

	bool ValidateBarrier(GpuTrackedResource* InResource, GpuResourceState NewState)
	{
		if (InResource->GetType() == GpuResourceType::Buffer)
		{
			GpuBuffer* Buffer = static_cast<GpuBuffer*>(InResource);
			if (EnumHasAllFlags(Buffer->GetUsage(), GpuBufferUsage::Uniform))
			{
				SH_LOG(LogRhiValidation, Error, TEXT("Barrier Error(Can not change the uniform buffe's state)"));
				return false;
			}
		}
		return ValidateGpuResourceState(NewState);
	}

	bool ValidateGpuResourceState(GpuResourceState InState)
	{
		if (EnumHasAnyFlags(InState, GpuResourceState::WriteMask) && EnumHasAnyFlags(InState, GpuResourceState::ReadMask))
		{
			SH_LOG(LogRhiValidation, Error, TEXT("GpuResourceState Error(Only a wirte state can be set)"));
			return false;
		}

		return true;
	}

	bool ValidateCreateTexture(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
	{
		if (EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::RenderTarget) && EnumHasAnyFlags(InTexDesc.Usage, GpuTextureUsage::ShaderResource) 
			&& InitState == GpuResourceState::Unknown)
		{
			SH_LOG(LogRhiValidation, Error, TEXT("CreateTexture Error(The InitState must be specified) for current texture usage."));
			return false;
		}
		return ValidateGpuResourceState(InitState);
	}

}
