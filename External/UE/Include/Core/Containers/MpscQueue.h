// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/TypeCompatibleBytes.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/Optional.h"
#include <atomic>

/** 
 * Fast multi-producer/single-consumer unbounded concurrent queue.
 * Based on http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
 */
template<typename T>
class TMpscQueue final
{
public:
	using ElementType = T;

	UE_NONCOPYABLE(TMpscQueue);

	TMpscQueue()
	{
		FNode* Sentinel = new FNode;
		Head.store(Sentinel, std::memory_order_relaxed);
		Tail = Sentinel;
	}

	~TMpscQueue()
	{
		FNode* Next = Tail->Next.load(std::memory_order_relaxed);

		// sentinel's value is already destroyed
		delete Tail;

		while (Next != nullptr)
		{
			Tail = Next;
			Next = Tail->Next.load(std::memory_order_relaxed);

			DestructItem((ElementType*)&Tail->Value);
			delete Tail;
		}
	}

	template <typename... ArgTypes>
	void Enqueue(ArgTypes&&... Args)
	{
		FNode* New = new FNode;
		new (&New->Value) ElementType(Forward<ArgTypes>(Args)...);

		FNode* Prev = Head.exchange(New, std::memory_order_acq_rel);
		Prev->Next.store(New, std::memory_order_release);
	}

	TOptional<ElementType> Dequeue()
	{
		FNode* Next = Tail->Next.load(std::memory_order_acquire);

		if (Next == nullptr)
		{
			return {};
		}

		ElementType* ValuePtr = (ElementType*)&Next->Value;
		TOptional<ElementType> Res{ MoveTemp(*ValuePtr) };
		DestructItem(ValuePtr);

		delete Tail; // current sentinel

		Tail = Next; // new sentinel
		return Res;
	}

private:
	struct FNode
	{
		std::atomic<FNode*> Next{ nullptr };
		TTypeCompatibleBytes<ElementType> Value;
	};

private:
	std::atomic<FNode*> Head; // accessed only by producers
	/*alignas(PLATFORM_CACHE_LINE_SIZE) */FNode* Tail; // accessed only by consumer, hence should be on a different cache line than `Head`
};
