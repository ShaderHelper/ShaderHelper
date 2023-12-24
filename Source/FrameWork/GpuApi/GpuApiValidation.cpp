#include "CommonHeader.h"
#include "GpuApiValidation.h"
#include "magic_enum.hpp"

namespace FRAMEWORK::GpuApi
{

	bool ValidateSetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		auto ValidateBindGroupNumber = [](GpuBindGroup* BindGroup, BindingGroupSlot ExpectedSlot)
		{
			if (BindGroup)
			{
				BindingGroupSlot GroupNumber = BindGroup->GetLayout()->GetGroupNumber();
				if (GroupNumber != ExpectedSlot) {
					SH_LOG(LogGpuApi, Error, TEXT("GpuApi::SetBindGroups Error(Mismatched BindingGroupSlot) - BindGroup%d : (%d), Expected : (%d)"), ExpectedSlot, GroupNumber, ExpectedSlot);
					return false;
				}
			}
			return true;
		};
		return ValidateBindGroupNumber(BindGroup0, 0) && ValidateBindGroupNumber(BindGroup1, 1) && ValidateBindGroupNumber(BindGroup2, 2) && ValidateBindGroupNumber(BindGroup3, 3);
	}

	static bool ValidateBindGroupResource(const ResourceBinding& InResourceBinding, BindingType LayoutEntryType)
	{
		GpuResource* GroupEntryResource = InResourceBinding.Resource;
		BindingSlot Slot = InResourceBinding.Slot;
		GpuResourceType GroupEntryType = GroupEntryResource->GetType();
		if (LayoutEntryType == BindingType::UniformBuffer)
		{
			if (GroupEntryType != GpuResourceType::Buffer)
			{
				SH_LOG(LogGpuApi, Error, TEXT("GpuApi::CreateBindGroup Error(Mismatched GpuResourceType)"
					" - Slot%d : (%s), Expected : (Buffer)"), Slot, ANSI_TO_TCHAR(magic_enum::enum_name(GroupEntryType).data()));
				return false;
			}
			else
			{
				GpuBufferUsage BufferUsage = static_cast<GpuBuffer*>(GroupEntryResource)->GetUsage();
				if (BufferUsage != GpuBufferUsage::PersistentUniform && BufferUsage != GpuBufferUsage::TemporaryUniform)
				{
					SH_LOG(LogGpuApi, Error, TEXT("GpuApi::CreateBindGroup Error(Mismatched GpuBufferUsage)"
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
		const TArray<ResourceBinding>& BindGroupResources = InBindGroupDesc.Resources;
		for (const auto& BindingLayoutEntry : LayoutDesc.Layouts)
		{
			BindingSlot LayoutSlot = BindingLayoutEntry.Slot;
			const ResourceBinding* BindGroupEntryPtr = BindGroupResources.FindByPredicate([LayoutSlot](const ResourceBinding& BindGroupEntry) {
				return BindGroupEntry.Slot == LayoutSlot;
				});
			if (BindGroupEntryPtr)
			{
				return ValidateBindGroupResource(*BindGroupEntryPtr, BindingLayoutEntry.Type);
			}
			else
			{
				SH_LOG(LogGpuApi, Error, TEXT("GpuApi::CreateBindGroup Error(Missing BindingSlot) - Not find the slot : (%d)"), LayoutSlot);
				return false;
			}
		}
		return true;
	}

    bool ValidateCreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
    {
        TArray<BindingSlot> Slots;
        for(const auto& BindingLayoutEntry : InBindGroupLayoutDesc.Layouts)
        {
            if(Slots.Contains(BindingLayoutEntry.Slot))
            {
                SH_LOG(LogGpuApi, Error, TEXT("GpuApi::CreateBindGroupLayout Error(Duplicated BindingSlot) -  slot detected: (%d)"), BindingLayoutEntry.Slot);
                return false;
            }
            Slots.Add(BindingLayoutEntry.Slot);
        }
        return true;
    }

	bool ValidateCreateRenderPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc)
	{
		auto ValidateBindGroupNumber = [](GpuBindGroupLayout* BindGroupLayout, BindingGroupSlot ExpectedSlot)
		{
			if (BindGroupLayout)
			{
				BindingGroupSlot GroupNumber = BindGroupLayout->GetGroupNumber();
				if (GroupNumber != ExpectedSlot) {
					SH_LOG(LogGpuApi, Error, TEXT("GpuApi::CreateRenderPipelineState Error(Mismatched BindingGroupSlot) - BindGroupLayout%d : (%d), Expected : (%d)"), ExpectedSlot, GroupNumber, ExpectedSlot);
					return false;
				}
			}
			return true;
		};
		return ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout0, 0) && ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout1, 1)
			&& ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout2, 2) && ValidateBindGroupNumber(InPipelineStateDesc.BindGroupLayout3, 3);
		return true;
	}

}
