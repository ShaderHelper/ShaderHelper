#include "CommonHeader.h"
#include "ShaderToyPin.h"
#include "GpuApi/GpuResourceHelper.h"
using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<GpuTexturePin>()
		.BaseClass<GraphPin>()
	)

	GLOBAL_REFLECTION_REGISTER(AddClass<SlotPin>()
		.BaseClass<GraphPin>()
	)

	void GpuTexturePin::Serialize(FArchive& Ar)
	{

	}

	void GpuTexturePin::LinkTo(GraphPin* TargetPin)
	{

	}

	GpuTexture* GpuTexturePin::GetValue() const
	{
		if (!Value) {
			return GpuResourceHelper::GetGlobalBlackTex();
		}
		return Value;
	}

	void SlotPin::Serialize(FArchive& Ar)
	{

	}

	void SlotPin::LinkTo(GraphPin* TargetPin)
	{

	}

}