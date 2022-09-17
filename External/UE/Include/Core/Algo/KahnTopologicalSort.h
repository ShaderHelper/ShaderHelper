// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Algo/Sort.h"
#include "Algo/Unique.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Misc/EnumClassFlags.h"
#include "Templates/Invoke.h"
#include "Templates/UnrealTemplate.h"
#include "Traits/ElementType.h"

namespace AlgoImpl
{
	/**
	 * KahnTopologicalSort converts vertices and edges from ElementType to indexes of the vertex in the original
	 * UniqueRange of ElementType. FKahnHandle is how vertices are represented during the graph algorithm
	 */
	typedef int32 FKahnHandle;

	/** Some variables shared with subfunctions */
	struct FKahnContext
	{
		TSet<FKahnHandle> RemainingVertices;
		TArray<TArray<FKahnHandle>> Referencers;
		TArray<TArray<FKahnHandle>> Dependencies;
		TArray<int32> DependencyCount;
		TSet<FKahnHandle> CycleVisited;
		TArray<FKahnHandle> CycleStack;
	};

	// Helper functions
	template <typename RangeType, typename GetElementDependenciesType>
	void KahnTopologicalSort_CreateWorkingGraph(FKahnContext& Context, RangeType& UniqueRange,
		GetElementDependenciesType GetElementDependencies, TSet<FKahnHandle>& OutInitialIndependents);
	FKahnHandle KahnTopologicalSort_PickMinimumCycleVertex(FKahnContext& Context);
}

namespace Algo
{
	/** Flags for behavior of TopologicalSort; see the function comment in TopologicalSort.h */
	enum class ETopologicalSort
	{
		None,
		AllowCycles,
	};
	ENUM_CLASS_FLAGS(ETopologicalSort);

	/** Public entrypoint. Implements Algo::TopologicalSort using the Kahn Topological Sort algorithm. */
	template <typename RangeType, typename GetElementDependenciesType>
	bool KahnTopologicalSort(RangeType& UniqueRange, GetElementDependenciesType GetElementDependencies,
		ETopologicalSort Flags)
	{
		using namespace AlgoImpl;
		using ElementType = typename TElementType<RangeType>::Type;

		FKahnContext Context;
		TSet<FKahnHandle> IndependentVertices;
		KahnTopologicalSort_CreateWorkingGraph(Context, UniqueRange,
			Forward<GetElementDependenciesType>(GetElementDependencies), IndependentVertices);

		// Initialize the graph search
		TArray<TArray<FKahnHandle>>& Referencers(Context.Referencers);
		TArray<TArray<FKahnHandle>>& Dependencies(Context.Dependencies);
		TArray<int32>& DependencyCount(Context.DependencyCount);
		TSet<FKahnHandle>& RemainingVertices(Context.RemainingVertices);
		TSet<FKahnHandle> NewIndependentVertices;
		TArray<FKahnHandle> SortedRange;
		int32 NumElements = Dependencies.Num();
		SortedRange.Reserve(NumElements);
		RemainingVertices.Reserve(NumElements);
		for (FKahnHandle Vertex = 0; Vertex < NumElements; ++Vertex)
		{
			RemainingVertices.Add(Vertex);
		}

		// Sort graph so that vertices with no dependencies (leaves) always go first.
		while (RemainingVertices.Num() > 0)
		{
			if (IndependentVertices.Num() == 0)
			{
				// If there are no independent vertices then there is a cycle in the graph
				if (!EnumHasAnyFlags(Flags, ETopologicalSort::AllowCycles))
				{
					return false;
				}

				// In the presence of a cycle, pick a vertex that minimizes the number of unvisited dependencies.
				FKahnHandle Vertex = KahnTopologicalSort_PickMinimumCycleVertex(Context);
				IndependentVertices.Add(Vertex);
				// Mark that we should no longer consider Vertex when looking for dependency-count==0 vertices.
				DependencyCount[Vertex] = INDEX_NONE;
			}

			NewIndependentVertices.Reset();
			for (FKahnHandle Vertex : IndependentVertices)
			{
				for (FKahnHandle Referencer : Referencers[Vertex])
				{
					int32& ReferencerDependencyCount = DependencyCount[Referencer];
					if (ReferencerDependencyCount == INDEX_NONE)
					{
						// Don't read or write dependencycount for referencers we removed due to a cycle
						continue;
					}
					check(ReferencerDependencyCount > 0);
					if (--ReferencerDependencyCount == 0)
					{
						NewIndependentVertices.Add(Referencer);
					}
				}
#if DO_CHECK
				int32 RemainingNum = RemainingVertices.Num();
#endif
				RemainingVertices.Remove(Vertex);
				check(RemainingVertices.Num() == RemainingNum - 1); // Confirm Vertex was in RemainingVertices
				SortedRange.Add(Vertex);
			}
			Swap(NewIndependentVertices, IndependentVertices);
		}

		// Shuffle the input according to the SortOrder found by the graph search
		TArray<ElementType> CopyOriginal;
		CopyOriginal.Reserve(NumElements);
		for (ElementType& Element : UniqueRange)
		{
			CopyOriginal.Add(MoveTemp(Element));
		}
		int32 SourceIndex = 0;
		for (ElementType& TargetElement : UniqueRange)
		{
			TargetElement = MoveTemp(CopyOriginal[SortedRange[SourceIndex++]]);
		}

		return true;
	}
}

namespace AlgoImpl
{
	/**
	 * Convert UniqueRange and GetElementDependencies into handles,
	 * dependency count, dependencies, and referencers
	 */
	template <typename RangeType, typename GetElementDependenciesType>
	inline void KahnTopologicalSort_CreateWorkingGraph(FKahnContext& Context, RangeType& UniqueRange, 
		GetElementDependenciesType GetElementDependencies, TSet<FKahnHandle>& OutInitialIndependents)
	{
		using ElementType = typename TElementType<RangeType>::Type;

		TArray<TArray<FKahnHandle>>& Referencers(Context.Referencers);
		TArray<TArray<FKahnHandle>>& Dependencies(Context.Dependencies);
		TArray<int32>& DependencyCount(Context.DependencyCount);

		int32 NumElements = GetNum(UniqueRange);
		TMap<ElementType, FKahnHandle> HandleOfElement;
		HandleOfElement.Reserve(NumElements);
		FKahnHandle Handle = 0;
		for (const ElementType& Element : UniqueRange)
		{
			FKahnHandle& ExistingHandle = HandleOfElement.FindOrAdd(Element, INDEX_NONE);
			check(ExistingHandle == INDEX_NONE);
			ExistingHandle = Handle++;
		}

		Referencers.SetNum(NumElements);
		Dependencies.SetNum(NumElements);
		DependencyCount.SetNum(NumElements);
		Handle = 0;
		for (const ElementType& Element : UniqueRange)
		{
			const auto& DependenciesInput = Invoke(GetElementDependencies, Element);
			TArray<FKahnHandle>& UniqueElementDependencies = Dependencies[Handle];

			for (const ElementType& Dependency : DependenciesInput)
			{
				FKahnHandle* DependencyHandle = HandleOfElement.Find(Dependency);
				if (DependencyHandle)
				{
					UniqueElementDependencies.Add(*DependencyHandle);
				}
			}
			Algo::Sort(UniqueElementDependencies);
			UniqueElementDependencies.SetNum(Algo::Unique(UniqueElementDependencies), false /* bAllowShrinking */);
			int32 NumUniqueDependencies = UniqueElementDependencies.Num();
			DependencyCount[Handle] = NumUniqueDependencies;
			if (NumUniqueDependencies == 0)
			{
				OutInitialIndependents.Add(Handle);
			}
			for (FKahnHandle DependencyHandle : UniqueElementDependencies)
			{
				TArray<FKahnHandle>& ElementReferencers = Referencers[DependencyHandle];
				ElementReferencers.Add(Handle);
			}
			++Handle;
		}
	}

	/**
	 * Called when there is a cycle (aka no vertices are independent). It finds a cycle and returns
	 * one of the vertices with the minimum number of dependencies in that cycle.
	 */
	inline FKahnHandle KahnTopologicalSort_PickMinimumCycleVertex(FKahnContext& Context)
	{
		TSet<FKahnHandle>& CycleVisited = Context.CycleVisited;
		TArray<FKahnHandle>& CycleStack = Context.CycleStack;
		TArray<TArray<FKahnHandle>>& Dependencies = Context.Dependencies;
		TSet<FKahnHandle>& RemainingVertices = Context.RemainingVertices;
		TArray<int32>& DependencyCount = Context.DependencyCount;

		int32 NumElements = Dependencies.Num();
		CycleVisited.Reset();
		CycleStack.Reset();
		FKahnHandle Current = INDEX_NONE;
		for (FKahnHandle RemainingVertex : RemainingVertices)
		{
			Current = RemainingVertex;
			break;
		}
		check(Current != INDEX_NONE);

		// Find a cycle by arbitrarily following dependencies until we revisit a vertex.
		CycleVisited.Add(Current);
		CycleStack.Add(Current);
		bool bAlreadyExists = false;
		while (!bAlreadyExists)
		{
			// Assert a dependency is found. PickMinimumCycleVertex is only called when every vertex has dependencies.
			TArray<FKahnHandle>& ElementDependencies = Dependencies[Current];
			FKahnHandle NextVertex = INDEX_NONE;
			for (int32 Index = 0; Index < ElementDependencies.Num();)
			{
				FKahnHandle TestVertex = ElementDependencies[Index];
				if (RemainingVertices.Contains(TestVertex))
				{
					NextVertex = TestVertex;
					break;
				}
				else
				{
					ElementDependencies.RemoveAtSwap(Index);
				}
			}
			check(NextVertex != INDEX_NONE);
			Current = NextVertex;
			CycleVisited.Add(Current, &bAlreadyExists);
			CycleStack.Add(Current);
		}

		// The cycle is everything on the stack after the first occurrence of Current.
		// Pick the minimum-dependency-count of the cycle vertices.
		int32 NumStack = CycleStack.Num();
		int32 MinCount = DependencyCount[Current];
		FKahnHandle MinVertex = Current;
		for (int32 IndexInStack = NumStack - 2; IndexInStack >= 0; --IndexInStack)
		{
			FKahnHandle VertexInStack = CycleStack[IndexInStack];
			if (VertexInStack == Current)
			{
				break;
			}
			int32 Count = DependencyCount[VertexInStack];
			if (Count < MinCount)
			{
				MinCount = Count;
				MinVertex = VertexInStack;
			}
		}
		return MinVertex;
	}
}