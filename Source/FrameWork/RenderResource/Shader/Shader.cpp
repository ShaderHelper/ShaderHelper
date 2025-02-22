#include "CommonHeader.h"
#include "Shader.h"

namespace FW
{

	void BindingContext::EnumerateBinding(TFunctionRef<void(const ResourceBinding&, const LayoutBinding&)> Func)
	{
		auto EnumerateGroup = [&](GpuBindGroup* InGroup){
			if (!InGroup) return;
			for (const auto& [Slot, ResourceBindingEntry] : InGroup->GetDesc().Resources)
			{
				const auto& LayoutBindingEntry = InGroup->GetLayout()->GetDesc().Layouts[Slot];
				Func(ResourceBindingEntry, LayoutBindingEntry);
			}
		};

		EnumerateGroup(GlobalBindGroup);
		EnumerateGroup(PassBindGroup);
		EnumerateGroup(ShaderBindGroup);
	}

}