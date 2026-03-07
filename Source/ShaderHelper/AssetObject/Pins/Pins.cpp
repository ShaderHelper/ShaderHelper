#include "CommonHeader.h"
#include "Pins.h"
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

	bool GpuTexturePin::CanAccept(GraphPin* SourcePin)
	{
		return DynamicCast<GpuTexturePin>(SourcePin) != nullptr;
	}

	void GpuTexturePin::Accept(GraphPin* SourcePin)
	{
		Value = static_cast<GpuTexturePin*>(SourcePin)->GetValue();
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
			static auto DefaultValue = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::RenderTargetWrite);
			Value = DefaultValue;
		}
		return Value;
	}

	REFLECTION_REGISTER(AddClass<GpuCubemapPin>("GpuCubemapPin")
		.BaseClass<GraphPin>()
	)

	void GpuCubemapPin::Serialize(FArchive& Ar)
	{
		GraphPin::Serialize(Ar);
	}

	bool GpuCubemapPin::CanAccept(GraphPin* SourcePin)
	{
		return DynamicCast<GpuCubemapPin>(SourcePin) != nullptr;
	}

	void GpuCubemapPin::Accept(GraphPin* SourcePin)
	{
		Value = static_cast<GpuCubemapPin*>(SourcePin)->GetValue();
	}

	void GpuCubemapPin::Refuse()
	{
		Value.SafeRelease();
	}

	void GpuCubemapPin::SetValue(TRefCountPtr<GpuTexture> InValue)
	{
		Value = MoveTemp(InValue);
		auto TargetPins = GetTargetPins();
		for (GraphPin* Pin : TargetPins)
		{
			Pin->Accept(this);
		}
	}

	GpuTexture* GpuCubemapPin::GetValue()
	{
		if (!Value) {
			TArray<uint8> RawData = { 0,0,0, 255 };
			GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared, RawData };
			static auto DefaultValue = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::RenderTargetWrite);
			Value = DefaultValue;
		}
		return Value;
	}

}
