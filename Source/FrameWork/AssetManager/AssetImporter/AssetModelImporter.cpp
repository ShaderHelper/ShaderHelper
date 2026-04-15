#include "CommonHeader.h"
#include "AssetModelImporter.h"
#include "App/App.h"
#include "Common/Util/Reflection.h"
#include "AssetObject/Model.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"

#include <fbxsdk.h>
#include <Misc/FileHelper.h>

namespace FW
{
	REFLECTION_REGISTER(AddClass<AssetModelImporter>()
								.BaseClass<AssetImporter>()
	)

	static FbxAMatrix GetNodeGeometryTransform(FbxNode* InNode)
	{
		return FbxAMatrix(
			InNode->GetGeometricTranslation(FbxNode::eSourcePivot),
			InNode->GetGeometricRotation(FbxNode::eSourcePivot),
			InNode->GetGeometricScaling(FbxNode::eSourcePivot));
	}

	static MeshData WeldVertices(MeshData InMeshData)
	{
		MeshData WeldedData;
		WeldedData.Indices.Reserve(InMeshData.Indices.Num());
		WeldedData.Vertices.Reserve(InMeshData.Vertices.Num());

		TMap<MeshVertex, uint32> VertexToIndex;
		for (uint32 Index : InMeshData.Indices)
		{
			const MeshVertex& Vertex = InMeshData.Vertices[Index];
			if (uint32* ExistingIndex = VertexToIndex.Find(Vertex))
			{
				WeldedData.Indices.Add(*ExistingIndex);
			}
			else
			{
				const uint32 NewIndex = WeldedData.Vertices.Num();
				WeldedData.Vertices.Add(Vertex);
				WeldedData.Indices.Add(NewIndex);
				VertexToIndex.Add(Vertex, NewIndex);
			}
		}

		return WeldedData;
	}

	static void ProcessNode(FbxNode* InNode, TArray<MeshData>& OutSubMeshes)
	{
		if (FbxMesh* FbxMeshObj = InNode->GetMesh())
		{
			MeshData Data;

			const FbxAMatrix MeshTransform = InNode->EvaluateGlobalTransform() * GetNodeGeometryTransform(InNode);
			const FbxAMatrix NormalTransform = MeshTransform.Inverse().Transpose();
			FbxVector4* ControlPoints = FbxMeshObj->GetControlPoints();

			const int UVSetCount = FMath::Min(FbxMeshObj->GetElementUVCount(), (int)MaxMeshUVSets);
			const char* UVSetNames[MaxMeshUVSets] = {};
			for (int i = 0; i < UVSetCount; ++i)
			{
				UVSetNames[i] = FbxMeshObj->GetElementUV(i)->GetName();
			}

			// Generate tangent/binormal data if not present (requires UVs)
			if (UVSetCount > 0 && FbxMeshObj->GetElementTangentCount() == 0)
			{
				FbxMeshObj->GenerateTangentsData(UVSetNames[0]);
			}

			FbxGeometryElementTangent* TangentElement = FbxMeshObj->GetElementTangentCount() > 0 ? FbxMeshObj->GetElementTangent(0) : nullptr;
			FbxGeometryElementBinormal* BinormalElement = FbxMeshObj->GetElementBinormalCount() > 0 ? FbxMeshObj->GetElementBinormal(0) : nullptr;

			int VertexCounter = 0;
			int PolygonCount = FbxMeshObj->GetPolygonCount();
			for (int Poly = 0; Poly < PolygonCount; ++Poly)
			{
				int PolySize = FbxMeshObj->GetPolygonSize(Poly);
				for (int Vert = 0; Vert < PolySize; ++Vert)
				{
					int ControlPointIndex = FbxMeshObj->GetPolygonVertex(Poly, Vert);
					FbxVector4 Pos = MeshTransform.MultT(ControlPoints[ControlPointIndex]);

					MeshVertex V;
					V.Position = Vector3f((float)Pos[0], (float)Pos[1], (float)Pos[2]);

					FbxVector4 Normal;
					if (FbxMeshObj->GetPolygonVertexNormal(Poly, Vert, Normal))
					{
						const FbxVector4 TransformedNormal = NormalTransform.MultT(FbxVector4(Normal[0], Normal[1], Normal[2], 0.0));
						V.Normal = FVector3f((float)TransformedNormal[0], (float)TransformedNormal[1], (float)TransformedNormal[2]).GetSafeNormal();
					}

					for (int UVIdx = 0; UVIdx < UVSetCount; ++UVIdx)
					{
						FbxVector2 UV;
						bool bUnmapped = false;
						if (FbxMeshObj->GetPolygonVertexUV(Poly, Vert, UVSetNames[UVIdx], UV, bUnmapped))
						{
							V.UVs[UVIdx] = Vector2f((float)UV[0], (float)UV[1]);
						}
					}

					if (TangentElement)
					{
						int TangentIndex = TangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint ? ControlPointIndex : VertexCounter;
						if (TangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						{
							TangentIndex = TangentElement->GetIndexArray().GetAt(TangentIndex);
						}
						FbxVector4 Tan = TangentElement->GetDirectArray().GetAt(TangentIndex);
						const FbxVector4 TransformedTangent = NormalTransform.MultT(FbxVector4(Tan[0], Tan[1], Tan[2], 0.0));
						FVector3f T = FVector3f((float)TransformedTangent[0], (float)TransformedTangent[1], (float)TransformedTangent[2]).GetSafeNormal();

						float Sign = 1.0f;
						if (BinormalElement)
						{
							int BinormalIndex = BinormalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint ? ControlPointIndex : VertexCounter;
							if (BinormalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
							{
								BinormalIndex = BinormalElement->GetIndexArray().GetAt(BinormalIndex);
							}
							FbxVector4 Bin = BinormalElement->GetDirectArray().GetAt(BinormalIndex);
							const FbxVector4 TransformedBinormal = NormalTransform.MultT(FbxVector4(Bin[0], Bin[1], Bin[2], 0.0));
							FVector3f B((float)TransformedBinormal[0], (float)TransformedBinormal[1], (float)TransformedBinormal[2]);
							Sign = FVector3f::DotProduct(FVector3f::CrossProduct(FVector3f(V.Normal), T), B) >= 0.0f ? 1.0f : -1.0f;
						}

						V.Tangent = Vector4f(T.X, T.Y, T.Z, Sign);
					}

					++VertexCounter;
					Data.Indices.Add(Data.Vertices.Num());
					Data.Vertices.Add(V);
				}
			}

			if (Data.Vertices.Num() > 0)
			{
				OutSubMeshes.Add(WeldVertices(MoveTemp(Data)));
			}
		}

		for (int i = 0; i < InNode->GetChildCount(); ++i)
		{
			ProcessNode(InNode->GetChild(i), OutSubMeshes);
		}
	}

	TUniquePtr<AssetObject> AssetModelImporter::CreateAssetObject(const FString& InFilePath)
	{
		FbxManager* Manager = FbxManager::Create();
		if (!Manager)
		{
			return nullptr;
		}

		FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
		Manager->SetIOSettings(IOSettings);

		FbxImporter* Importer = FbxImporter::Create(Manager, "");
		if (!Importer->Initialize(TCHAR_TO_UTF8(*InFilePath), -1, Manager->GetIOSettings()))
		{
			const FString ErrorMessage = FString::Printf(TEXT("FBX importer failed to initialize:\n%s"), UTF8_TO_TCHAR(Importer->GetStatus().GetErrorString()));
			MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), FText::FromString(ErrorMessage));
			Importer->Destroy();
			Manager->Destroy();
			return nullptr;
		}

		FbxScene* Scene = FbxScene::Create(Manager, "ImportScene");
		if (!Importer->Import(Scene))
		{
			const FString ErrorMessage = FString::Printf(TEXT("FBX import failed:\n%s"), UTF8_TO_TCHAR(Importer->GetStatus().GetErrorString()));
			MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), FText::FromString(ErrorMessage));
			Importer->Destroy();
			Manager->Destroy();
			return nullptr;
		}
		Importer->Destroy();

		FbxAxisSystem::DirectX.DeepConvertScene(Scene);

		FbxGeometryConverter Converter(Manager);
		Converter.Triangulate(Scene, true);

		TArray<MeshData> SubMeshes;
		ProcessNode(Scene->GetRootNode(), SubMeshes);

		Manager->Destroy();

		if (SubMeshes.Num() == 0)
		{
			return nullptr;
		}

		return MakeUnique<Model>(MoveTemp(SubMeshes));
	}

	TArray<FString> AssetModelImporter::SupportFileExts() const
	{
		return { "fbx", "obj" };
	}

	MetaType* AssetModelImporter::SupportAsset()
	{
		return GetMetaType<Model>();
	}
}
