#pragma once
#include "AssetObject/Graph.h"
#include "GpuApi/GpuTexture.h"

namespace SH
{
	class GpuTexturePin : public FRAMEWORK::GraphPin
	{
		REFLECTION_TYPE(GpuTexturePin)
	public:
		using GraphPin::GraphPin;

		void Serialize(FArchive& Ar) override;
		void LinkTo(GraphPin* TargetPin) override;
		FRAMEWORK::GpuTexture* GetValue() const;

	private:
		TRefCountPtr<FRAMEWORK::GpuTexture> Value;
	};

	class SlotPin : public FRAMEWORK::GraphPin
	{
		REFLECTION_TYPE(SlotPin)
	public:
		using GraphPin::GraphPin;

		void Serialize(FArchive& Ar) override;
		void LinkTo(GraphPin* TargetPin) override;
	};
}