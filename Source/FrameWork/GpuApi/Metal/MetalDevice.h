#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
    inline MTL::Device* GDevice;
    inline MTL::CommandQueue* GCommandQueue;
    inline MTL::CounterSet* GTimestampCounterSet = nullptr;
    inline bool GSupportStageBoundaryCounter = false;

    class MetalQuerySet : public GpuQuerySet
    {
    public:
        MetalQuerySet(uint32 InCount, NS::SharedPtr<MTL::CounterSampleBuffer> InSampleBuffer);
        double GetTimestampPeriodNs() const override;
        void ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps) override;
        MTL::CounterSampleBuffer* GetSampleBuffer() const { return SampleBuffer.get(); }
    private:
        NS::SharedPtr<MTL::CounterSampleBuffer> SampleBuffer;
    };

    extern void InitMetalCore();
}
