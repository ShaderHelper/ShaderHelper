// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/TypeCompatibleBytes.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/Optional.h"
#include <atomic>

/**
 * Fast single-producer/single-consumer unbounded concurrent queue. Doesn't free memory until destruction but recycles consumed items.
 * Based on http://www.1024cores.net/home/lock-free-algorithms/queues/unbounded-spsc-queue
 */
template<typename T>
class TSpscQueue final
{
public:
	using ElementType = T;

	UE_NONCOPYABLE(TSpscQueue);

	TSpscQueue()
	{
		FNode* Node = new FNode;
		Tail.store(Node, std::memory_order_relaxed);
		Head = First = TailCopy = Node;
	}

	~TSpscQueue()
	{
		FNode* Node = First;
		FNode* LocalTail = Tail.load(std::memory_order_relaxed);

		// Delete all nodes which are the sentinel or unoccupied
		bool bContinue = false;
		do
		{
			FNode* Next = Node->Next.load(std::memory_order_relaxed);
			bContinue = Node != LocalTail;
			delete Node;
			Node = Next;
		} while (bContinue);

		// Delete all nodes which are occupied, destroying the element first
		while (Node != nullptr)
		{
			FNode* Next = Node->Next.load(std::memory_order_relaxed);
			DestructItem((ElementType*)&Node->Value);
			delete Node;
			Node = Next;
		}
	}

	template <typename... ArgTypes>
	void Enqueue(ArgTypes&&... Args)
	{
		FNode* Node = AllocNode();
		new (&Node->Value) ElementType(Forward<ArgTypes>(Args)...);

		Head->Next.store(Node, std::memory_order_release);
		Head = Node;
	}

	// returns empty TOptional if queue is empty 
	TOptional<ElementType> Dequeue()
	{
		FNode* LocalTail = Tail.load(std::memory_order_relaxed);
		FNode* LocalTailNext = LocalTail->Next.load(std::memory_order_acquire);
		if (LocalTailNext == nullptr)
		{
			return {};
		}

		ElementType* TailNextValue = (ElementType*)&LocalTailNext->Value;
		TOptional<ElementType> Value{ MoveTemp(*TailNextValue) };
		DestructItem(TailNextValue);

		Tail.store(LocalTailNext, std::memory_order_release);
		return Value;
	}

private:
	struct FNode
	{
		std::atomic<FNode*> Next{ nullptr };
		TTypeCompatibleBytes<ElementType> Value;
	};

private:
	FNode* AllocNode()
	{
		// first tries to allocate node from internal node cache, 
		// if attempt fails, allocates node via ::operator new() 

		auto AllocFromCache = [this]()
		{
			FNode* Node = First;
			First = First->Next;
			Node->Next.store(nullptr, std::memory_order_relaxed);
			return Node;
		};

		if (First != TailCopy)
		{
			return AllocFromCache();
		}

		TailCopy = Tail.load(std::memory_order_acquire);
		if (First != TailCopy)
		{
			return AllocFromCache();
		}

		return new FNode();
	}

private:
	// consumer part 
	// accessed mainly by consumer, infrequently by producer 
#ifndef PLATFORM_MAC // some projects are still built on macOS before v.10.14 that doesn't have aligned new/delete operators
	/*alignas(PLATFORM_CACHE_LINE_SIZE) */std::atomic<FNode*> Tail; // tail of the queue 
#else
	std::atomic<FNode*> Tail; // tail of the queue
#endif
	// producer part 
	// accessed only by producer 
#ifndef PLATFORM_MAC
	/*alignas(PLATFORM_CACHE_LINE_SIZE) */FNode* Head; // head of the queue 
#else
	FNode* Head; // head of the queue
#endif
	FNode* First; // last unused node (tail of node cache) 
	FNode* TailCopy; // helper (points somewhere between First and Tail)
};