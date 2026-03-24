#include "CommonHeader.h"
#include "MetalDevice.h"
#include "GpuApi/GpuFeature.h"

namespace FW
{
    void InitMetalCore()
    {
        GDevice = MTL::CreateSystemDefaultDevice();
		GpuFeature::Support16bitType = true;

//		if(@available(macOS 13.3, *))
//		{
//			GDx12Device->setShouldMaximizeConcurrentCompilation(true);
//		}

		// Check timestamp counter support
		{
			NS::Array* CounterSets = GDevice->counterSets();
			if (CounterSets)
			{
				for (NS::UInteger i = 0; i < CounterSets->count(); i++)
				{
					MTL::CounterSet* CS = static_cast<MTL::CounterSet*>(CounterSets->object(i));
					if (CS->name()->isEqualToString(MTL::CommonCounterSetTimestamp))
					{
						GTimestampCounterSet = CS;
					    GpuFeature::SupportTimestampQuery = true;
						break;
					}
				}
			}
		}

        GCommandQueue = GDevice->newCommandQueue();
    }

    MetalQuerySet::MetalQuerySet(uint32 InCount, NS::SharedPtr<MTL::CounterSampleBuffer> InSampleBuffer)
        : GpuQuerySet(InCount), SampleBuffer(std::move(InSampleBuffer))
    {
    }

    double MetalQuerySet::GetTimestampPeriodNs() const
    {
        // Metal timestamps are already in nanoseconds
        return 1.0;
    }

    void MetalQuerySet::ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps)
    {
        OutTimestamps.SetNum(QueryCount);
        NS::Range Range = NS::Range::Make(FirstQuery, QueryCount);
        NS::Data* Data = SampleBuffer->resolveCounterRange(Range);

        const MTL::CounterResultTimestamp* Results = static_cast<const MTL::CounterResultTimestamp*>(Data->bytes());
        for (uint32 i = 0; i < QueryCount; i++)
        {
            OutTimestamps[i] = Results[i].timestamp;
        }
    }
}
