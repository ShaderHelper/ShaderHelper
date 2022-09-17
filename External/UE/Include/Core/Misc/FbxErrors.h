// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "UObject/NameTypes.h"
#include "Templates/SharedPointer.h"
#include "Logging/TokenizedMessage.h"


/**
 * This file contains known map errors that can be referenced by name.
 * Documentation for these errors is assumed to lie in UDN at: Engine\Documentation\Source\Shared\Editor\MapErrors
 */
struct CORE_API FFbxErrors
{
	/** Generic */
	static FLazyName Generic_ImportingNewObjectFailed;

	static FLazyName Generic_ReimportingObjectFailed;

	static FLazyName Generic_LoadingSceneFailed;

	static FLazyName Generic_InvalidCharacterInName;

	static FLazyName Generic_SameNameAssetExists;

	static FLazyName Generic_SameNameAssetOverriding;

	static FLazyName Generic_CannotDeleteReferenced;

	static FLazyName Generic_FBXFileParseFailed;

	static FLazyName Generic_MeshNotFound;

	static FLazyName Generic_CannotDetectImportType;

	/** Mesh Generic **/

	static FLazyName Generic_Mesh_NoGeometry;

	static FLazyName Generic_Mesh_SmallGeometry;

	static FLazyName Generic_Mesh_TriangulationFailed;

	static FLazyName Generic_Mesh_ConvertSmoothingGroupFailed;

	static FLazyName Generic_Mesh_UnsupportingSmoothingGroup;

	static FLazyName Generic_Mesh_MaterialIndexInconsistency;

	static FLazyName Generic_Mesh_MeshNotFound;

	static FLazyName Generic_Mesh_NoSmoothingGroup;

	static FLazyName Generic_Mesh_LOD_InvalidIndex;

	static FLazyName Generic_Mesh_LOD_NoFileSelected;

	static FLazyName Generic_Mesh_LOD_MultipleFilesSelected;

	static FLazyName Generic_Mesh_SkinxxNameError;

	static FLazyName Generic_Mesh_TooManyLODs;

	static FLazyName Generic_Mesh_TangentsComputeError;

	static FLazyName Generic_Mesh_NoReductionModuleAvailable;

	static FLazyName Generic_Mesh_TooMuchUVChannels;

	/** Static Mesh **/
	static FLazyName StaticMesh_TooManyMaterials;

	static FLazyName StaticMesh_UVSetLayoutProblem;

	static FLazyName StaticMesh_NoTriangles;

	static FLazyName StaticMesh_BuildError;

	static FLazyName StaticMesh_AllTrianglesDegenerate;

	static FLazyName StaticMesh_AdjacencyOptionForced;

	/** SkeletalMesh **/
	static FLazyName SkeletalMesh_DifferentRoots;

	static FLazyName SkeletalMesh_DuplicateBones;

	static FLazyName SkeletalMesh_NoInfluences;

	static FLazyName SkeletalMesh_TooManyInfluences;

	static FLazyName SkeletalMesh_RestoreSortingMismatchedStrips;

	static FLazyName SkeletalMesh_RestoreSortingNoSectionMatch;

	static FLazyName SkeletalMesh_RestoreSortingForSectionNumber;

	static FLazyName SkeletalMesh_NoMeshFoundOnRoot;
	
	static FLazyName SkeletalMesh_InvalidRoot;

	static FLazyName SkeletalMesh_InvalidBone;

	static FLazyName SkeletalMesh_InvalidNode;

	static FLazyName SkeletalMesh_NoWeightsOnDeformer;

	static FLazyName SkeletalMesh_NoBindPoseInScene;

	static FLazyName SkeletalMesh_NoAssociatedCluster;

	static FLazyName SkeletalMesh_NoBoneFound;

	static FLazyName SkeletalMesh_InvalidBindPose;

	static FLazyName SkeletalMesh_MultipleRoots;

	static FLazyName SkeletalMesh_BonesAreMissingFromBindPose;

	static FLazyName SkeletalMesh_VertMissingInfluences;

	static FLazyName SkeletalMesh_SectionWithNoTriangle;

	static FLazyName SkeletalMesh_TooManyVertices;

	static FLazyName SkeletalMesh_FailedToCreatePhyscisAsset;

	static FLazyName SkeletalMesh_SkeletonRecreateError;

	static FLazyName SkeletalMesh_ExceedsMaxBoneCount;

	static FLazyName SkeletalMesh_NoUVSet;

	static FLazyName SkeletalMesh_LOD_MissingBone;

	static FLazyName SkeletalMesh_LOD_FailedToImport;

	static FLazyName SkeletalMesh_LOD_RootNameIncorrect;

	static FLazyName SkeletalMesh_LOD_BonesDoNotMatch;

	static FLazyName SkeletalMesh_LOD_IncorrectParent;

	static FLazyName SkeletalMesh_LOD_HasSoftVerts;

	static FLazyName SkeletalMesh_LOD_MissingSocketBone;

	static FLazyName SkeletalMesh_LOD_MissingMorphTarget;

	static FLazyName SkeletalMesh_FillImportDataFailed;

	static FLazyName SkeletalMesh_InvalidPosition;

	/** Animation **/
	static FLazyName Animation_CouldNotFindRootTrack;

	static FLazyName Animation_CouldNotBuildSkeleton;

	static FLazyName Animation_CouldNotFindTrack;

	static FLazyName Animation_ZeroLength;

	static FLazyName Animation_RootTrackMismatch;

	static FLazyName Animation_DuplicatedBone;

	static FLazyName Animation_MissingBones;

	static FLazyName Animation_InvalidData;

	static FLazyName Animation_TransformError;

	static FLazyName Animation_DifferentLength;

	static FLazyName Animation_CurveNotFound;
};

/**
 * Map error specific message token.
 */
class FFbxErrorToken : public FDocumentationToken
{
public:
	/** Factory method, tokens can only be constructed as shared refs */
	CORE_API static TSharedRef<FFbxErrorToken> Create( const FName& InErrorName )
	{
		return MakeShareable(new FFbxErrorToken(InErrorName));
	}

private:
	/** Private constructor */
	CORE_API FFbxErrorToken( const FName& InErrorName );
};
