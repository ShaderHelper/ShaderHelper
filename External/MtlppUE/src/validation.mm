/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Foundation/NSObject.h>
#include <Metal/MTLResource.h>
#include <Metal/MTLBuffer.h>
#include <libkern/OSAtomicQueue.h>
#include "validation.hpp"
#include "command_encoder.hpp"
#include "blit_command_encoder.hpp"
#include "compute_command_encoder.hpp"
#include "parallel_render_command_encoder.hpp"
#include "render_command_encoder.hpp"
#include "command_buffer_fence.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "debugger.hpp"
#include <set>
#include <map>
#include <vector>

MTLPP_BEGIN

#if MTLPP_CONFIG_VALIDATE

struct ScopedNSLock
{
	ScopedNSLock(id<NSLocking> L)
	: Lock(L)
	{
		assert(Lock);
		[Lock lock];
	}
	
	~ScopedNSLock()
	{
		assert(Lock);
		[Lock unlock];
	}
	
	id<NSLocking> Lock;
};


@interface ResourceValidationTableImpl : NSObject
{
@public
	id<MTLResource> Resource;
	ns::AutoReleased<mtlpp::Device> Device;
}
-(instancetype)initWithResource:(id<MTLResource>)Res;
@end

@implementation ResourceValidationTableImpl
-(instancetype)initWithResource:(id<MTLResource>)Res
{
	id Self = [super init];
	if (Self)
	{
		assert(Res);
		Resource = Res;
		Device = Res.device;
	}
	return Self;
}
-(void)dealloc
{
	mtlpp::DeviceValidationTable DevTable = Device.GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
	
	DevTable.ValidateUsage(Resource, mtlpp::ResourceUsage::Read);
	DevTable.ValidateUsage(Resource, mtlpp::ResourceUsage::Write);
	
	[super dealloc];
}
@end

class BufferShadowMap
{
public:
    BufferShadowMap()
    : Size(0)
    , Map(nullptr)
    {
        
    }
    
    ~BufferShadowMap()
    {
        if (Map)
        {
            NSUInteger Len = (Size / 16);
            for (NSUInteger i = 0; i < Len; i++)
            {
                if (Map[i] != 0)
                {
                    fprintf(stderr, "Error: Buffer %p deleted with active %u active sub-allocation\n", Resource, (unsigned)i);
                    mtlpp::Break();
                    break;
                }
            }
            
            delete [] Map;
        }
    }
    
    void Init(id<MTLBuffer> Res)
    {
        Resource = Res;
        Size = Res.length;
    }
    
    bool Allocate(ns::Range const& Range)
    {
        NSUInteger Loc = (Range.Location / 16);
        NSUInteger Len = (Range.Length / 16);
        
        if (!Map)
        {
            NSUInteger Alloc = Size / 16;
            uint8_t* Mem = new uint8_t[Alloc];
            memset (Mem,0,Alloc);
            void* Val = __sync_val_compare_and_swap((void**)&Map, nullptr, Mem);
            if (Val != nullptr)
            {
                delete [] Mem;
            }
        }
        assert(Map);
        
        bool bNotAlreadyMapped = true;
        for (NSUInteger i = Loc; i < (Loc + Len); ++i)
        {
            uint8_t Val = __sync_val_compare_and_swap(&Map[i], 0, 1);
            bNotAlreadyMapped &= (Val == 0);
        }
        
        return bNotAlreadyMapped;
    }
    
    bool Release(ns::Range const& Range)
    {
        if (Map)
        {
            NSUInteger Loc = (Range.Location / 16);
            NSUInteger Len = (Range.Length / 16);
            
            bool bAlreadyMapped = true;
            for (NSUInteger i = Loc; i < (Loc + Len); ++i)
            {
                uint8_t Val = __sync_val_compare_and_swap(&Map[i], 1, 0);
                bAlreadyMapped &= (Val != 0);
            }
            
            return bAlreadyMapped;
        }
        else
        {
            return false;
        }
    }
    
private:
    uint64_t Size;
    id<MTLBuffer> Resource;
    volatile uint8_t* Map;
};

@interface BufferValidationTableImpl : ResourceValidationTableImpl
{
@public
    BufferShadowMap Map;
}
-(instancetype)initWithResource:(id<MTLBuffer>)Res;
@end

@implementation BufferValidationTableImpl
-(instancetype)initWithResource:(id<MTLBuffer>)Res
{
	id Self = [super initWithResource:Res];
	if (Self)
    {
        Map.Init(Res);
	}
	return Self;
}
@end

@interface DeviceValidationTableImpl : NSObject
{
@public
	std::set<CommandBufferValidationTableImpl*> EnqueuedCommandBufferValidators;
}
@property (retain) 	NSRecursiveLock* Lock;
@end

@implementation DeviceValidationTableImpl
@synthesize Lock;
-(instancetype)init
{
	id Self = [super init];
	if (Self)
	{
		Lock = [NSRecursiveLock new];
	}
	return Self;
}
@end

@interface CommandEncoderValidationTableImpl : NSObject
{
@public
	std::set<ns::Protocol<id<MTLResource>>::type> ReadResources;
	std::set<ns::Protocol<id<MTLResource>>::type> WriteResources;
	
	std::map<ns::Protocol<id<MTLBuffer>>::type, std::vector<ns::Range>> ReadBufferRanges;
	std::map<ns::Protocol<id<MTLBuffer>>::type, std::vector<ns::Range>> WriteBufferRanges;
}
@end

@implementation CommandEncoderValidationTableImpl
@end

struct CommandEncoderValidationPtr
{
	CommandEncoderValidationPtr()
	: Next(nullptr)
	, Encoder(nullptr)
	{
	}
	
	CommandEncoderValidationPtr* Next;
	mtlpp::CommandEncoderValidationTable Encoder;
};

template<typename T>
class lockless_lifo
{
	struct Link
	{
		Link(T const& V)
		: Next(nullptr)
		, Value(V)
		{
		}
		Link* Next;
		T Value;
	};
	
public:
	struct iterator
	{
		iterator() : Ptr(nullptr) {}
		
		T operator*()
		{
			if (Ptr)
			{
				return Ptr->Value;
			}
			else
			{
				return T();
			}
		}
		
		T* operator->()
		{
			if (Ptr)
				return &Ptr->Value;
			else
				return nullptr;
		}
		
		T const* operator->() const
		{
			if (Ptr)
				return &Ptr->Value;
			else
				return nullptr;
		}
		
		iterator& operator++()
		{
			if (Ptr)
			{
				Ptr = Ptr->Next;
			}
			return *this;
		}
		
		bool operator!=(iterator const& it)
		{
			return (Ptr != it.Ptr);
		}
		
		Link* Ptr;
	};
	
	iterator begin()
	{
		iterator it;
		it.Ptr = (Link*)Head.opaque1;
		return it;
	}
	
	iterator end()
	{
		return iterator();
	}
	
public:
	lockless_lifo()
	: Head(OS_ATOMIC_QUEUE_INIT)
	{
	}
	
	void push(T const& V)
	{
		static size_t offset = offsetof(Link, Next);
		OSAtomicEnqueue(&Head, new Link(V), offset);
	}
	
	T pop(void)
	{
		T Value;
		static size_t offset = offsetof(Link, Next);
		Link* L = OSAtomicDequeue(&Head, offset);
		if (L)
		{
			Value = L->Value;
			delete L;
		}
		return Value;
	}
	
	~lockless_lifo()
	{
		static size_t offset = offsetof(Link, Next);
		while (Link* L = (Link*)OSAtomicDequeue(&Head, offset))
		{
			delete L;
		}
	}
private:
	OSQueueHead Head;
};

@interface CommandBufferValidationTableImpl : NSObject
{
@public
	mtlpp::CommandBufferFence Fence;
	lockless_lifo<mtlpp::CommandEncoderValidationTable> EncoderValidators;
	BOOL bEnqueued;
}
@property (retain) DeviceValidationTableImpl* DeviceTable;
@end

@implementation CommandBufferValidationTableImpl
@synthesize DeviceTable;
-(instancetype)init
{
	id Self = [super init];
	if (Self)
	{
		bEnqueued = false;
	}
	return Self;
}
-(void)dealloc
{
	if (DeviceTable && DeviceTable.Lock)
	{
		ScopedNSLock Lock(DeviceTable.Lock);
		DeviceTable->EnqueuedCommandBufferValidators.erase(self);
	}
	
	[super dealloc];
}
@end

@interface ParallelEncoderValidationTableImpl : NSObject
{
@public
	CommandBufferValidationTableImpl* CommandBufferValidator;
}
@end

@implementation ParallelEncoderValidationTableImpl
@end

namespace mtlpp
{
	char const* CommandEncoderValidationTable::kTableAssociationKey = "CommandEncoderValidationTable::kTableAssociationKey";
	char const* CommandBufferValidationTable::kTableAssociationKey = "CommandBufferValidationTable::kTableAssociationKey";
	char const* DeviceValidationTable::kTableAssociationKey = "DeviceValidationTable::kTableAssociationKey";
	char const* ResourceValidationTable::kTableAssociationKey = "ResourceValidationTable::kTableAssociationKey";
	char const* BufferValidationTable::kTableAssociationKey = ResourceValidationTable::kTableAssociationKey;
	char const* ParallelEncoderValidationTable::kTableAssociationKey = "ParallelEncoderValidationTable::kTableAssociationKey";

	ResourceValidationTable::ResourceValidationTable(Resource& Resource)
	: ns::Object<ResourceValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[ResourceValidationTableImpl alloc] initWithResource:Resource], ns::Ownership::Assign)
	{
		Resource.SetAssociatedObject(ResourceValidationTable::kTableAssociationKey, *this);
	}

	ResourceValidationTable::ResourceValidationTable(ResourceValidationTableImpl* Table)
	: ns::Object<ResourceValidationTableImpl*, ns::CallingConvention::ObjectiveC>(Table)
	{
		
	}
	
	bool ResourceValidationTable::ValidateUsage(NSUInteger Usage) const
	{
		mtlpp::DeviceValidationTable DevTable = m_ptr->Device.GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
		return DevTable.ValidateUsage(m_ptr->Resource, Usage);
	}
	
	BufferValidationTable::BufferValidationTable::BufferValidationTable(Buffer& Resource)
	: ns::Object<BufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[BufferValidationTableImpl alloc] initWithResource:Resource], ns::Ownership::Assign)
	{
		Resource.SetAssociatedObject(BufferValidationTable::kTableAssociationKey, *this);
	}
	
	BufferValidationTable::BufferValidationTable(BufferValidationTableImpl* Table)
	: ns::Object<BufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>(Table)
	{
		
	}
	
	BufferValidationTable::BufferValidationTable(ns::Ownership retain)
	: ns::Object<BufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>(retain)
	{
		
	}
	
	bool BufferValidationTable::ValidateUsage(NSUInteger Usage) const
	{
		mtlpp::DeviceValidationTable DevTable = m_ptr->Device.GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
		return DevTable.ValidateUsage((id<MTLBuffer>)m_ptr->Resource, Usage);
	}
	
	bool BufferValidationTable::ValidateUsage(NSUInteger Usage, ns::Range Range) const
	{
		mtlpp::DeviceValidationTable DevTable = m_ptr->Device.GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
		return DevTable.ValidateUsage((id<MTLBuffer>)m_ptr->Resource, Range, Usage);
	}
	
	void BufferValidationTable::AllocateRange(ns::Range Range)
	{
		if (m_ptr->Map.Allocate(Range))
		{
			mtlpp::DeviceValidationTable DevTable = m_ptr->Device.GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
			DevTable.ValidateUsage((id<MTLBuffer>)m_ptr->Resource, Range, mtlpp::ResourceUsage::Write);
		}
		else
		{
			fprintf(stderr, "Error: Allocating range %u:%u from buffer %p that intersects allocated range.\n", (unsigned)Range.Location, (unsigned)Range.Length, m_ptr->Resource);
			mtlpp::Break();
		}
	}
	
	void BufferValidationTable::ReleaseRange(ns::Range Range)
	{
        @autoreleasepool
        {
            if (m_ptr->Map.Release(Range))
            {
                mtlpp::DeviceValidationTable DevTable = m_ptr->Device.GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
                DevTable.ValidateUsage((id<MTLBuffer>)m_ptr->Resource, Range, mtlpp::ResourceUsage::Read);
                DevTable.ValidateUsage((id<MTLBuffer>)m_ptr->Resource, Range, mtlpp::ResourceUsage::Write);
            }
        }
	}
	
	void BufferValidationTable::ReleaseAllRanges(ns::Range Range)
	{
		ReleaseRange(Range);
	}
	
	ParallelEncoderValidationTable::ParallelEncoderValidationTable(ParallelRenderCommandEncoder& Encoder)
	: ns::Object<ParallelEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[ParallelEncoderValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Encoder.SetAssociatedObject(ParallelEncoderValidationTable::kTableAssociationKey, *this);
	}
	ParallelEncoderValidationTable::ParallelEncoderValidationTable(ParallelEncoderValidationTableImpl* Table)
	: ns::Object<ParallelEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>(Table)
	{
		
	}
		
	void ParallelEncoderValidationTable::AddEncoderValidator(RenderCommandEncoder& Encoder)
	{
		CommandBufferValidationTable(GetPtr()->CommandBufferValidator).AddEncoderValidator(Encoder);
	}
	
	CommandEncoderValidationTable::CommandEncoderValidationTable(BlitCommandEncoder& Encoder)
	: ns::Object<CommandEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[CommandEncoderValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Encoder.SetAssociatedObject(CommandEncoderValidationTable::kTableAssociationKey, *this);
	}
	
	CommandEncoderValidationTable::CommandEncoderValidationTable(ComputeCommandEncoder& Encoder)
	: ns::Object<CommandEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[CommandEncoderValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Encoder.SetAssociatedObject(CommandEncoderValidationTable::kTableAssociationKey, *this);
	}
	
	CommandEncoderValidationTable::CommandEncoderValidationTable(RenderCommandEncoder& Encoder)
	: ns::Object<CommandEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[CommandEncoderValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Encoder.SetAssociatedObject(CommandEncoderValidationTable::kTableAssociationKey, *this);
	}
	
	CommandEncoderValidationTable::CommandEncoderValidationTable(ParallelRenderCommandEncoder& Encoder)
	: ns::Object<CommandEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[CommandEncoderValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Encoder.SetAssociatedObject(CommandEncoderValidationTable::kTableAssociationKey, *this);
	}
	
	CommandEncoderValidationTable::CommandEncoderValidationTable(CommandEncoderValidationTableImpl* Table)
	: ns::Object<CommandEncoderValidationTableImpl*, ns::CallingConvention::ObjectiveC>(Table)
	{
		
	}
	
	void CommandEncoderValidationTable::UseResource(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage)
	{
		if (mtlpp::ResourceUsage::Write & Usage)
		{
			auto It = m_ptr->WriteBufferRanges.find(Resource);
			if (It == m_ptr->WriteBufferRanges.end())
			{
				It = m_ptr->WriteBufferRanges.emplace(Resource, std::vector<ns::Range>()).first;
			}
			
			bool bFound = false;
			for (ns::Range const& range : It->second)
			{
				if (NSEqualRanges(NSMakeRange(Range.Location, Range.Length), NSMakeRange(range.Location, range.Length)))
				{
					bFound = true;
					break;
				}
			}
			
			if (!bFound)
			{
				It->second.push_back(Range);
			}
		}
		else
		{
			auto It = m_ptr->ReadBufferRanges.find(Resource);
			if (It == m_ptr->ReadBufferRanges.end())
			{
				It = m_ptr->ReadBufferRanges.emplace(Resource, std::vector<ns::Range>()).first;
			}
			
			bool bFound = false;
			for (ns::Range const& range : It->second)
			{
				if (NSEqualRanges(NSMakeRange(Range.Location, Range.Length), NSMakeRange(range.Location, range.Length)))
				{
					bFound = true;
					break;
				}
			}
			
			if (!bFound)
			{
				It->second.push_back(Range);
			}
		}
	}
	
	bool CommandEncoderValidationTable::ValidateUsage(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage) const
	{
		bool bOK = true;
		ns::Range Overlap;
		if (mtlpp::ResourceUsage::Write & Usage)
		{
			auto It = m_ptr->ReadBufferRanges.find(Resource);
			if (It != m_ptr->ReadBufferRanges.end())
			{
				for (ns::Range const& range : It->second)
				{
					if (NSIntersectionRange(NSMakeRange(Range.Location, Range.Length), NSMakeRange(range.Location, range.Length)).length > 0)
					{
						bOK = false;
						Overlap = range;
						break;
					}
				}
			}
		}
		else
		{
			auto It = m_ptr->WriteBufferRanges.find(Resource);
			if (It != m_ptr->WriteBufferRanges.end())
			{
				for (ns::Range const& range : It->second)
				{
					if (NSIntersectionRange(NSMakeRange(Range.Location, Range.Length), NSMakeRange(range.Location, range.Length)).length > 0)
					{
						bOK = false;
						Overlap = range;
						break;
					}
				}
			}
		}
		if (!bOK)
		{
			fprintf(stderr, "Error: Resource usage %s on range (%llu:%llu) while in use on the GPU for range (%llu:%llu), encoder %p: %s\n", Usage == ResourceUsage::Write ? "Write" : "Read", (uint64_t)Range.Location, (uint64_t)Range.Length, (uint64_t)Overlap.Location, (uint64_t)Overlap.Length, m_ptr, [[Resource debugDescription] UTF8String]);
			Break();
		}
		return bOK;
	}
	
	void CommandEncoderValidationTable::UseResource(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage)
	{
		if (mtlpp::ResourceUsage::Write & Usage)
		{
			m_ptr->WriteResources.insert(Resource);
		}
		else
		{
			m_ptr->ReadResources.insert(Resource);
		}
	}
	
	bool CommandEncoderValidationTable::ValidateUsage(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage) const
	{
		bool bOK = true;
		if (mtlpp::ResourceUsage::Write & Usage)
		{
			bOK = (m_ptr->ReadResources.find(Resource) == m_ptr->ReadResources.end());
		}
		else
		{
			bOK = (m_ptr->WriteResources.find(Resource) == m_ptr->WriteResources.end());
		}
		if (!bOK)
		{
			fprintf(stderr, "Error: Resource usage %s while in use on the GPU: %s\n", Usage == ResourceUsage::Write ? "Write" : "Read", [[Resource debugDescription] UTF8String]);
			Break();
		}
		return bOK;
	}
	
	CommandBufferValidationTable::CommandBufferValidationTable(CommandBuffer& Buffer)
	: ns::Object<CommandBufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[CommandBufferValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Buffer.SetAssociatedObject(CommandBufferValidationTable::kTableAssociationKey, *this);
		
		GetPtr()->Fence = Buffer.GetCompletionFence();
		
		mtlpp::DeviceValidationTable DevTable = Buffer.GetDevice().GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
		GetPtr().DeviceTable = DevTable.GetPtr();

		Buffer.AddCompletedHandler(^(const mtlpp::CommandBuffer & CmdBuf) {
			CommandBufferValidationTable CmdBufTable = CmdBuf.GetAssociatedObject<CommandBufferValidationTable>(CommandBufferValidationTable::kTableAssociationKey);
			mtlpp::DeviceValidationTable DevTable = CmdBuf.GetDevice().GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
			ScopedNSLock Lock(DevTable.GetPtr().Lock);
			DevTable.GetPtr()->EnqueuedCommandBufferValidators.erase(CmdBufTable.GetPtr());
			const_cast<mtlpp::CommandBuffer & >(CmdBuf).ClearAssociatedObject(CommandBufferValidationTable::kTableAssociationKey);
		});
	}
	
	CommandBufferValidationTable::CommandBufferValidationTable(CommandBufferValidationTableImpl* Table)
	: ns::Object<CommandBufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>(Table)
	{
		
	}
	
	CommandBufferValidationTable::CommandBufferValidationTable(ns::Ownership retain)
	: ns::Object<CommandBufferValidationTableImpl*, ns::CallingConvention::ObjectiveC>(retain)
	{
		
	}
	
	void CommandBufferValidationTable::AddEncoderValidator(BlitCommandEncoder& Encoder)
	{
		CommandEncoderValidationTable Validator(Encoder);
		m_ptr->EncoderValidators.push(Validator);
	}
	
	void CommandBufferValidationTable::AddEncoderValidator(ComputeCommandEncoder& Encoder)
	{
		CommandEncoderValidationTable Validator(Encoder);
		m_ptr->EncoderValidators.push(Validator);
	}
	
	CommandEncoderValidationTable CommandBufferValidationTable::AddEncoderValidator(RenderCommandEncoder& Encoder)
	{
		CommandEncoderValidationTable Validator(Encoder);
		m_ptr->EncoderValidators.push(Validator);
		return Validator;
	}
	
	CommandEncoderValidationTable CommandBufferValidationTable::AddEncoderValidator(ParallelRenderCommandEncoder& Encoder)
	{
		ParallelEncoderValidationTable ParallelValidator(Encoder);
		ParallelValidator.GetPtr()->CommandBufferValidator = m_ptr;
		
		CommandEncoderValidationTable Validator(Encoder);
		m_ptr->EncoderValidators.push(Validator);
		return Validator;
	}
	
	bool CommandBufferValidationTable::ValidateUsage(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage) const
	{
		bool bOK = true;
		if (!m_ptr->Fence.Wait(0))
		{
			for (auto It = m_ptr->EncoderValidators.begin(); bOK && It != m_ptr->EncoderValidators.end(); ++It)
			{
				bOK = It->ValidateUsage(Resource, Range, Usage);
			}
		}
		return bOK;
	}
	
	bool CommandBufferValidationTable::ValidateUsage(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage) const
	{
		bool bOK = true;
		if (!m_ptr->Fence.Wait(0))
		{
			for (auto It = m_ptr->EncoderValidators.begin(); bOK && It != m_ptr->EncoderValidators.end(); ++It)
			{
				bOK = It->ValidateUsage(Resource, Usage);
			}
		}
		return bOK;
	}
	
	void CommandBufferValidationTable::Enqueue(CommandBuffer& Buffer)
	{
		mtlpp::DeviceValidationTable DevTable = Buffer.GetDevice().GetAssociatedObject<mtlpp::DeviceValidationTable>(mtlpp::DeviceValidationTable::kTableAssociationKey);
		ScopedNSLock DevLock(DevTable.GetPtr().Lock);
		if (!m_ptr->bEnqueued)
		{
			m_ptr->bEnqueued = true;
			DevTable.Enqueue(Buffer);
		}
	}
	
	DeviceValidationTable::DeviceValidationTable(Device& Device)
	: ns::Object<DeviceValidationTableImpl*, ns::CallingConvention::ObjectiveC>([[DeviceValidationTableImpl alloc] init], ns::Ownership::Assign)
	{
		Device.SetAssociatedObject(DeviceValidationTable::kTableAssociationKey, *this);
	}

	DeviceValidationTable::DeviceValidationTable(DeviceValidationTableImpl* Table)
	: ns::Object<DeviceValidationTableImpl*, ns::CallingConvention::ObjectiveC>(Table)
	{
		
	}
	
	void DeviceValidationTable::Enqueue(CommandBuffer& Buffer)
	{
		ScopedNSLock Lock(m_ptr.Lock);
		CommandBufferValidationTable Table = Buffer.GetAssociatedObject<CommandBufferValidationTable>(CommandBufferValidationTable::kTableAssociationKey);
		m_ptr->EnqueuedCommandBufferValidators.insert(Table.GetPtr());
	}
	
	bool DeviceValidationTable::ValidateUsage(ns::Protocol<id<MTLBuffer>>::type Resource, ns::Range Range, NSUInteger Usage) const
	{
		bool bOK = true;
		ScopedNSLock Lock(m_ptr.Lock);
		for (auto It = m_ptr->EnqueuedCommandBufferValidators.rbegin(); bOK && It != m_ptr->EnqueuedCommandBufferValidators.rend(); ++It)
		{
			bOK = ns::AutoReleased<CommandBufferValidationTable>(*It).ValidateUsage(Resource, Range, Usage);
		}
		return bOK;
	}
	
	bool DeviceValidationTable::ValidateUsage(ns::Protocol<id<MTLResource>>::type Resource, NSUInteger Usage) const
	{
		bool bOK = true;
		ScopedNSLock Lock(m_ptr.Lock);
		for (auto It = m_ptr->EnqueuedCommandBufferValidators.rbegin(); bOK && It != m_ptr->EnqueuedCommandBufferValidators.rend(); ++It)
		{
			bOK = ns::AutoReleased<CommandBufferValidationTable>(*It).ValidateUsage(Resource, Usage);
		}
		return bOK;
	}
}
#endif

MTLPP_END
