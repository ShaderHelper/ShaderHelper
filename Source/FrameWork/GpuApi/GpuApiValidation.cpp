#include "CommonHeader.h"
#include "GpuApiValidation.h"
#include "magic_enum.hpp"

namespace FRAMEWORK
{

	bool ValidateSetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		auto ValidateBindGroupNumber = [](GpuBindGroup* BindGroup, BindingGroupSlot ExpectedSlot)
		{
			if (BindGroup)
			{
				BindingGroupSlot GroupNumber = BindGroup->GetLayout()->GetGroupNumber();
				if (GroupNumber != ExpectedSlot) {
					SH_LOG(LogGpuApi, Error, TEXT("GpuRhi::SetBindGroups Error(Mismatched BindingGroupSlot) - BindGroup%d : (%d), Expected : (%d)"), ExpectedSlot, GroupNumber, ExpectedSlot);
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
				SH_LOG(LogGpuApi, Error, TEXT("GpuRhi::CreateBindGroup Error(Mismatched GpuResourceType)"
					" - Slot%d : (%s), Expected : (Buffer)"), Slot, ANSI_TO_TCHAR(magic_enum::enum_name(GroupEntryType).data()));
				return false;
			}
			else
			{
				GpuBufferUsage BufferUsage = static_cast<GpuBuffer*>(GroupEntryResource)->GetUsage();
				if (BufferUsage != GpuBufferUsage::PersistentUniform && BufferUsage != GpuBufferUsage::TemporaryUniform)
				{
					SH_LOG(LogGpuApi, Error, TEXT("GpuRhi::CreateBindGroup Error(Mismatched GpuBufferUsage)"
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
		for (const auto& [Slot, LayoutBindingEntry] : LayoutDesc.Layouts)
		{
			if (auto* ResourceBindingEntry = InBindGroupDesc.Resources.Find(Slot))
			{
				return ValidateBindGroupResource(Slot, *ResourceBindingEntry, LayoutBindingEntry.Type);
			}
			else
			{
				SH_LOG(LogGpuApi, Error, TEXT("GpuRhi::CreateBindGroup Error(Missing BindingSlot) - Not find the slot : (%d)"), Slot);
				return false;
			}
		}
		return true;
	}

    bool ValidateCreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
    {
        TArray<BindingSlot> Slots;
        for(const auto& [Slot, _] : InBindGroupLayoutDesc.Layouts)
        {
            if(Slots.Contains(Slot))
            {
                SH_LOG(LogGpuApi, Error, TEXT("GpuRhi::CreateBindGroupLayout Error(Duplicated BindingSlot) -  slot detected: (%d)"), Slot);
                return false;
            }
            Slots.Add(Slot);
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
					SH_LOG(LogGpuApi, Error, TEXT("GpuRhi::CreateRenderPipelineState Error(Mismatched BindingGroupSlot) - BindGroupLayout%d : (%d), Expected : (%d)"), ExpectedSlot, GroupNumber, ExpectedSlot);
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
