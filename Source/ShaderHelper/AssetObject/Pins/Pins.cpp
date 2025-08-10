#include "CommonHeader.h"
#include "Pins.h"
#include "GpuApi/GpuResourceHelper.h"
#include "GpuApi/GpuRhi.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<GpuTexturePin>("GpuTexturePin")
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

    void GpuTexturePin::SetValue(TRefCountPtr<GpuTexture> InValue)
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
			
			TArray<uint8> RawData = {0,0,0, 255};
			GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared, RawData};
			Value = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::RenderTargetWrite);
		}
		return Value;
	}

}
