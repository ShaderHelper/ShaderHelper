// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/GarbageCollectionHistory.h"
#include "HAL/ThreadHeartBeat.h"

/** Search mode flags */
enum class EReferenceChainSearchMode
{
	// Returns all reference chains found
	Default = 0,
	// Returns only reference chains from external objects
	ExternalOnly = 1 << 0,
	// Returns only the shortest reference chain for each rooted object
	Shortest = 1 << 1,
	// Returns only the longest reference chain for each rooted object
	Longest = 1 << 2,
	// Returns only the direct referencers
	Direct = 1 << 3,
	// Returns complete chains. (Ignoring non GC objects)
	FullChain = 1 << 4,

	// Print results
	PrintResults = 1 << 16,
	// Print ALL results (in some cases there may be thousands of reference chains)
	PrintAllResults = 1 << 17,
};

ENUM_CLASS_FLAGS(EReferenceChainSearchMode);


class FReferenceChainSearch
{
	// Reference chain searching is a very slow operation.
	// Suspend the hang and hitch detectors for the lifetime of this instance.
	FSlowHeartBeatScope SuspendHeartBeat;
	FDisableHitchDetectorScope SuspendGameThreadHitch;

public:

	/** Type of reference */
	enum class EReferenceType
	{
		Unknown = 0,
		Property = 1,
		AddReferencedObjects
	};

	/** Extended information about a reference */
	template <typename T>
	struct TReferenceInfo
	{
		// Maximum number of stack frames to keep for AddReferencedObjects function calls
		static constexpr uint32 MaxStackFrames = 30;

		/** Object that is being referenced */
		T* Object;
		/** Type of reference to the object being referenced */
		EReferenceType Type;
		/** Name of the object or property that is referencing this object */
		FName ReferencerName;
		uint64 StackFrames[MaxStackFrames];
		int32 NumStackFrames;

		/** Default ctor */
		TReferenceInfo()
			: Object(nullptr)
			, Type(EReferenceType::Unknown)
		{
			InitStackFrames(nullptr, 0);
		}

		/** Simple refernce constructor. Probably will be filled with more info later */
		TReferenceInfo(T* InObject)
			: Object(InObject)
			, Type(EReferenceType::Unknown)
		{
			InitStackFrames(nullptr, 0);
		}

		/** Full reference infor constructor */
		TReferenceInfo(T* InObject, EReferenceType InType, const FName& InReferencerName, uint64* InStackFrames, int32 InNumStackFrames)
			: Object(InObject)
			, Type(InType)
			, ReferencerName(InReferencerName)
		{
			InitStackFrames(InStackFrames, InNumStackFrames);
		}

		bool operator == (const TReferenceInfo& Other) const
		{
			return Object == Other.Object;
		}

		friend uint32 GetTypeHash(const TReferenceInfo& Info)
		{
			return GetTypeHash(Info.Object);
		}

		/** Dumps this reference info to string. Does not include the object being referenced */
		FString ToString() const
		{
			if (Type == EReferenceType::Property)
			{
				return FString::Printf(TEXT("->%s"), *ReferencerName.ToString());
			}
			else if (Type == EReferenceType::AddReferencedObjects)
			{
				if (!ReferencerName.IsNone())
				{
					return FString::Printf(TEXT("::AddReferencedObjects(): %s"), *ReferencerName.ToString());
				}
				else
				{
					return TEXT("::AddReferencedObjects()");
				}
			}
			return FString();
		}
	private:

		void InitStackFrames(uint64* InStackFrames,int32 InNumStackFrames)
		{
			check(InNumStackFrames <= MaxStackFrames);
			NumStackFrames = InNumStackFrames;
			FMemory::Memset(StackFrames, 0, sizeof(StackFrames));
			if (InStackFrames && InNumStackFrames)
			{
				FMemory::Memcpy(StackFrames, InStackFrames, InNumStackFrames * sizeof(uint64));
			}
		}
	};

	/** Single node in the reference graph */
	struct FGraphNode
	{
		/** Object pointer */
		UE_DEPRECATED(5.0, "Direct Object reference has been deprecated. Use ObjectInfo member variable instead.")
		UObject* Object = nullptr;
		/** Object pointer */
		FGCObjectInfo* ObjectInfo = nullptr;
		/** Objects referenced by this object with reference info */
		TSet< TReferenceInfo<FGraphNode> > ReferencedObjects;
		/** Objects that have references to this object */
		TSet<FGraphNode*> ReferencedByObjects;
		/** Non-zero if this node has been already visited during reference search */
		int32 Visited = 0;
	};

	/** Convenience type definitions to avoid template hell */
	typedef TReferenceInfo<FGCObjectInfo> FObjectReferenceInfo;
	typedef TReferenceInfo<FGraphNode> FNodeReferenceInfo;

	/** Reference chain. The first object in the list is the target object and the last object is a root object */
	class FReferenceChain
	{
		friend class FReferenceChainSearch;

		/** Nodes in this reference chain. The first node is the target object and the last one is a root object */
		TArray<FGraphNode*> Nodes;
		/** Reference information for Nodes */
		TArray<FNodeReferenceInfo> ReferenceInfos;

		/** Fills this chain with extended reference info for each node */
		void FillReferenceInfo();

	public:
		FReferenceChain() {}
		FReferenceChain(int32 ReserveDepth)
		{
			Nodes.Reserve(ReserveDepth);
		}

		/** Adds a new node to the chain */
		void AddNode(FGraphNode* InNode)
		{
			Nodes.Add(InNode);
		}
		void InsertNode(FGraphNode* InNode)
		{
			Nodes.Insert(InNode, 0);
		}
		/** Gets a node from the chain */
		FGraphNode* GetNode(int32 NodeIndex) const
		{
			return Nodes[NodeIndex];
		}
		FGraphNode* GetRootNode() const
		{
			return Nodes.Last();
		}
		/** Returns the number of nodes in the chain */
		int32 Num() const
		{
			return Nodes.Num();
		}
		/** Returns a duplicate of this chain */
		FReferenceChain* Split()
		{
			FReferenceChain* NewChain = new FReferenceChain(*this);
			return NewChain;
		}
		/** Checks if this chain contains the specified node */
		bool Contains(const FGraphNode* InNode) const
		{
			return Nodes.Contains(InNode);
		}
		/** Gets extended reference info for the specified node index */
		const FNodeReferenceInfo& GetReferenceInfo(int32 NodeIndex) const
		{
			return ReferenceInfos[NodeIndex];
		}
		/** Check if this reference chain represents an external reference (the root is not in target object) */
		bool IsExternal() const;
	};

	/** Parameters passed to callback function when printing results */
	struct FCallbackParams
	{
		/** Referenced object */
		FGCObjectInfo* Object = nullptr;
		/** Object that is referencing the current object */
		FGCObjectInfo* Referencer = nullptr;
		/** Information about the type of reference (Referencer -> Object) */
		const FNodeReferenceInfo* ReferenceInfo = nullptr;
		/** For use when outputting custom information: current indent */
		int32 Indent = 0;
		/** Output device used for printing */
		FOutputDevice* Out = nullptr;
	};

	/** Constructs a new search engine and finds references to the specified object */
	COREUOBJECT_API explicit FReferenceChainSearch(UObject* InObjectToFindReferencesTo, EReferenceChainSearchMode Mode = EReferenceChainSearchMode::PrintResults);

	/** Constructs a new search engine but does not find references to any objects until one of the PerformSearch*() functions is called */
	COREUOBJECT_API explicit FReferenceChainSearch(EReferenceChainSearchMode Mode);

	/** Destructor */
	COREUOBJECT_API ~FReferenceChainSearch();

#if ENABLE_GC_HISTORY
	/** Searches for references in a previous GC run snapshot temporarily acquiring its object info */
	COREUOBJECT_API void PerformSearchFromGCSnapshot(UObject* InObjectToFindReferencesTo, FGCSnapshot& InSnapshot);
#endif //ENABLE_GC_HISTORY

	/**
	 * Dumps results to log
	 * @param bDumpAllChains - if set to false, the output will be trimmed to the first 100 reference chains
	 */
	COREUOBJECT_API void PrintResults(bool bDumpAllChains = false) const;

	/**
	 * Dumps results to log
	 * @param ReferenceCallback - function called when processing each reference, if true is returned the next reference will be processed otherwise printing will be aborted
	 * @param bDumpAllChains - if set to false, the output will be trimmed to the first 100 reference chains
	 */
	COREUOBJECT_API void PrintResults(TFunctionRef<bool(FCallbackParams& Params)> ReferenceCallback, bool bDumpAllChains = false) const;

	/** Returns a string with a short report explaining the root path, will contain newlines */
	COREUOBJECT_API FString GetRootPath() const;

	/** 
	 * Returns a string with a short report explaining the root path, will contain newlines 
	 * @param ReferenceCallback - function called when processing each reference, if true is returned the next reference will be processed otherwise printing will be aborted
	 */
	COREUOBJECT_API FString GetRootPath(TFunctionRef<bool(FCallbackParams& Params)> ReferenceCallback) const;

	/** Returns all reference chains */
	COREUOBJECT_API const TArray<FReferenceChain*>& GetReferenceChains() const
	{
		return ReferenceChains;
	}

private:

	/** The object we're going to look for references to */
	UObject* ObjectToFindReferencesTo = nullptr;
	FGCObjectInfo* ObjectInfoToFindReferencesTo = nullptr;

	/** Search mode and options */
	EReferenceChainSearchMode SearchMode = EReferenceChainSearchMode::Default;

	/** All reference chain found during the search */
	TArray<FReferenceChain*> ReferenceChains;
	/** All nodes created during the search */
	TMap<FGCObjectInfo*, FGraphNode*> AllNodes;
	/** Maps UObject pointers to object info structs */
	TMap<UObject*, FGCObjectInfo*> ObjectToInfoMap;

	/** Performs the search */
	void PerformSearch();

	/** Performs the search */
	void FindDirectReferencesForObjects();

	/** Frees memory */
	void Cleanup();

	/** Tries to find a node for an object and if it doesn't exists creates a new one and returns it */
	FGraphNode* FindOrAddNode(UObject* InObjectToFindNodeFor);
	FGraphNode* FindOrAddNode(FGCObjectInfo* InObjectInfo);

	/** Builds reference chains */
	static int32 BuildReferenceChains(FGraphNode* TargetNode, TArray<FReferenceChain*>& ProducedChains, int32 ChainDepth, const int32 VisitCounter, EReferenceChainSearchMode SearchMode);
	/** Builds reference chains */
	static void BuildReferenceChains(FGraphNode* TargetNode, TArray<FReferenceChain*>& AllChains, EReferenceChainSearchMode SearchMode);
	/** Builds reference chains for direct references only */
	static void BuildReferenceChainsForDirectReferences(FGraphNode* TargetNode, TArray<FReferenceChain*>& AllChains, EReferenceChainSearchMode SearchMode);
	/** Leaves only chains with unique root objects */
	static void RemoveChainsWithDuplicatedRoots(TArray<FReferenceChain*>& AllChains);
	/** Leaves only unique chains */
	static void RemoveDuplicatedChains(TArray<FReferenceChain*>& AllChains);

	/** Returns a string with all flags (we care about) set on an object */
	static FString GetObjectFlags(FGCObjectInfo* InObject);
	/** Dumps a reference chain to log */
	static void DumpChain(FReferenceChainSearch::FReferenceChain* Chain, TFunctionRef<bool(FCallbackParams& Params)> ReferenceCallback, FOutputDevice& Out);
};
