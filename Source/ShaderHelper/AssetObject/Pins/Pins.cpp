#include "CommonHeader.h"
#include "Pins.h"
#include "GpuApi/GpuRhi.h"

using namespace FW;

namespace SH
{
	namespace
	{
		TRefCountPtr<GpuTexture> CreateDefaultTexture()
		{
			TArray<uint8> RawData = {0, 0, 0, 255};
			GpuTextureDesc Desc{ 1, 1, GpuFormat::B8G8R8A8_UNORM, GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared, RawData };
			return GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::RenderTargetWrite);
		}

		TRefCountPtr<GpuTexture> CreateDefaultCubemap()
		{
			TArray<uint8> RawData;
			RawData.SetNumZeroed(4 * 6);
			for (uint32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
			{
				RawData[FaceIndex * 4 + 3] = 255;
			}

			GpuTextureDesc Desc{};
			Desc.Width = 1;
			Desc.Height = 1;
			Desc.Format = GpuFormat::B8G8R8A8_UNORM;
			Desc.Usage = GpuTextureUsage::ShaderResource;
			Desc.Dimension = GpuTextureDimension::TexCube;
			Desc.InitialData = RawData;
			return GGpuRhi->CreateTexture(Desc);
		}

		TRefCountPtr<GpuTexture> CreateDefaultVolumeTexture()
		{
			TArray<uint8> RawData = {0, 0, 0, 255};

			GpuTextureDesc Desc{};
			Desc.Width = 1;
			Desc.Height = 1;
			Desc.Depth = 1;
			Desc.Format = GpuFormat::B8G8R8A8_UNORM;
			Desc.Usage = GpuTextureUsage::ShaderResource;
			Desc.Dimension = GpuTextureDimension::Tex3D;
			Desc.InitialData = RawData;
			return GGpuRhi->CreateTexture(MoveTemp(Desc));
		}
	}

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
		if (Value)
		{
			return Value;
		}

		if (!DefaultValue)
		{
			DefaultValue = CreateDefaultTexture();
		}
		return DefaultValue;
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
		if (Value)
		{
			return Value;
		}

		if (!DefaultValue)
		{
			DefaultValue = CreateDefaultCubemap();
		}
		return DefaultValue;
	}

	REFLECTION_REGISTER(AddClass<GpuTexture3DPin>("GpuTexture3DPin")
		.BaseClass<GraphPin>()
	)

	void GpuTexture3DPin::Serialize(FArchive& Ar)
	{
		GraphPin::Serialize(Ar);
	}

	bool GpuTexture3DPin::CanAccept(GraphPin* SourcePin)
	{
		return DynamicCast<GpuTexture3DPin>(SourcePin) != nullptr;
	}

	void GpuTexture3DPin::Accept(GraphPin* SourcePin)
	{
		Value = static_cast<GpuTexture3DPin*>(SourcePin)->GetValue();
	}

	void GpuTexture3DPin::Refuse()
	{
		Value.SafeRelease();
	}

	void GpuTexture3DPin::SetValue(TRefCountPtr<GpuTexture> InValue)
	{
		Value = MoveTemp(InValue);
		auto TargetPins = GetTargetPins();
		for (GraphPin* Pin : TargetPins)
		{
			Pin->Accept(this);
		}
	}

	GpuTexture* GpuTexture3DPin::GetValue()
	{
		if (Value)
		{
			return Value;
		}

		if (!DefaultValue)
		{
			DefaultValue = CreateDefaultVolumeTexture();
		}
		return DefaultValue;
	}

}
