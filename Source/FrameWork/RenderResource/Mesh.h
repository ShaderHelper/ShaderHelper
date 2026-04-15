#pragma once

#include "GpuApi/GpuBuffer.h"

namespace FW
{
	static constexpr uint32 MaxMeshUVSets = 4;

	struct MeshVertex
	{
		Vector3f Position;
		Vector3f Normal;
		Vector2f UVs[MaxMeshUVSets];
		Vector4f Color = Vector4f(1.0f);
		Vector4f Tangent = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);

		bool operator==(const MeshVertex& Other) const
		{
			if (Position != Other.Position || Normal != Other.Normal || Color != Other.Color || Tangent != Other.Tangent)
				return false;
			for (uint32 i = 0; i < MaxMeshUVSets; ++i)
				if (UVs[i] != Other.UVs[i]) return false;
			return true;
		}

		friend uint32 GetTypeHash(const MeshVertex& Vertex)
		{
			uint32 Hash = GetTypeHash(Vertex.Position);
			Hash = HashCombine(Hash, GetTypeHash(Vertex.Normal));
			for (uint32 i = 0; i < MaxMeshUVSets; ++i)
				Hash = HashCombine(Hash, GetTypeHash(Vertex.UVs[i]));
			Hash = HashCombine(Hash, GetTypeHash(Vertex.Color));
			Hash = HashCombine(Hash, GetTypeHash(Vertex.Tangent));
			return Hash;
		}

		friend FArchive& operator<<(FArchive& Ar, MeshVertex& InVertex)
		{
			Ar << InVertex.Position;
			Ar << InVertex.Normal;
			for (uint32 i = 0; i < MaxMeshUVSets; ++i)
				Ar << InVertex.UVs[i];
			Ar << InVertex.Color;
			Ar << InVertex.Tangent;
			return Ar;
		}
	};

	struct MeshData
	{
		TArray<MeshVertex> Vertices;
		TArray<uint32> Indices;

		friend FArchive& operator<<(FArchive& Ar, MeshData& InMeshData)
		{
			Ar << InMeshData.Vertices;
			Ar << InMeshData.Indices;
			return Ar;
		}
	};

	struct MeshBuffers
	{
		TRefCountPtr<GpuBuffer> VertexBuffer;
		TRefCountPtr<GpuBuffer> IndexBuffer;
		uint32 VertexCount = 0;
		uint32 IndexCount = 0;
	};

	FRAMEWORK_API MeshData CreateQuad();
	FRAMEWORK_API MeshData CreateCube();
	FRAMEWORK_API MeshData CreateSphere(uint32 SliceCount = 32, uint32 StackCount = 16);
	FRAMEWORK_API MeshBuffers UploadMesh(const MeshData& InMeshData);
}
