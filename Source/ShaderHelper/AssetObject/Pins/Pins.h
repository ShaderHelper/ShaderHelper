#pragma once
#include "AssetObject/Graph.h"
#include "GpuApi/GpuTexture.h"

namespace SH
{
	class GpuTexturePin : public FW::GraphPin
	{
		REFLECTION_TYPE(GpuTexturePin)
	public:
		using GraphPin::GraphPin;

		void Serialize(FArchive& Ar) override;
		bool Accept(FW::GraphPin* SourcePin) override;
        void Refuse() override;
		FLinearColor GetPinColor() const override { return FLinearColor{0.24f, 0.7f, 0.44f}; }

        void SetValue(TRefCountPtr<FW::GpuTexture> InValue);
		FW::GpuTexture* GetValue();

	private:
		TRefCountPtr<FW::GpuTexture> Value;
	};

}
