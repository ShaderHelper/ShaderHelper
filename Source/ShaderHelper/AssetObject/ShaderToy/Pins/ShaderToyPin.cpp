#include "CommonHeader.h"
#include "ShaderToyPin.h"
#include "GpuApi/GpuResourceHelper.h"
using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<GpuTexturePin>()
		.BaseClass<GraphPin>()
	)

    REFLECTION_REGISTER(AddClass<ChannelPin>()
		.BaseClass<GraphPin>()
	)

	void GpuTexturePin::Serialize(FArchive& Ar)
	{
		GraphPin::Serialize(Ar);
	}

	bool GpuTexturePin::Accept(GraphPin* TargetPin)
	{
		return true;
	}

	GpuTexture* GpuTexturePin::GetValue() const
	{
		if (!Value) {
			return GpuResourceHelper::GetGlobalBlackTex();
		}
		return Value;
	}

	void ChannelPin::Serialize(FArchive& Ar)
	{
		GraphPin::Serialize(Ar);
	}

	bool ChannelPin::Accept(GraphPin* TargetPin)
	{
		return true;
	}

}
