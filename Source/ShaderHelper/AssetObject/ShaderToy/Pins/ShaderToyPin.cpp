#include "CommonHeader.h"
#include "ShaderToyPin.h"
#include "GpuApi/GpuResourceHelper.h"
#include "GpuApi/GpuRhi.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<GpuTexturePin>("GpuTexturePin")
		.BaseClass<GraphPin>()
	)

    REFLECTION_REGISTER(AddClass<ChannelPin>("ChannelPin")
		.BaseClass<GraphPin>()
	)

	void GpuTexturePin::Serialize(FArchive& Ar)
	{
		GraphPin::Serialize(Ar);
	}

	bool GpuTexturePin::Accept(GraphPin* SourcePin)
	{
        if(auto* SrcTexPin = DynamicCast<GpuTexturePin>(SourcePin))
        {
            Value = SrcTexPin->GetValue();
            return true;
        }
		return false;
	}

    void GpuTexturePin::Refuse()
    {
        Value.SafeRelease();
    }

    void GpuTexturePin::SetValue(TRefCountPtr<FW::GpuTexture> InValue)
    {
        Value = MoveTemp(InValue);
        //Refresh the pin pipe
        auto TargetPins = GetTargetPins();
        for(GraphPin* Pin : TargetPins)
        {
            Pin->Accept(this);
        }
    }

	GpuTexture* GpuTexturePin::GetValue()
	{
		if (!Value) {
			Value = GpuResourceHelper::GetGlobalBlackTex();
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
