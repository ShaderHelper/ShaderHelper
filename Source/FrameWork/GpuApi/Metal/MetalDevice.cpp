#include "CommonHeader.h"
#include "MetalDevice.h"
#include "GpuApi/GpuFeature.h"

namespace FW
{
    void InitMetalCore()
    {
        GDevice = MTL::CreateSystemDefaultDevice();

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
						GSupportStageBoundaryCounter = GDevice->supportsCounterSampling(MTL::CounterSamplingPointAtStageBoundary);
						break;
					}
				}
			}
		}

        GCommandQueue = GDevice->newCommandQueue();
    }
}
