#pragma once

#include "GpuApi/GpuBuffer.h"

namespace FW
{
	struct MeshVertex
	{
		Vector3f Position;
		Vector3f Normal;
		Vector2f UV;
		Vector4f Color = Vector4f(1.0f);
		Vector4f Tangent = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);

		bool operator==(const MeshVertex& Other) const
		{
			return Position == Other.Position
				&& Normal == Other.Normal
				&& UV == Other.UV
				&& Color == Other.Color
				&& Tangent == Other.Tangent;
		}

		friend uint32 GetTypeHash(const MeshVertex& Vertex)
		{
			uint32 Hash = GetTypeHash(Vertex.Position);
			Hash = HashCombine(Hash, GetTypeHash(Vertex.Normal));
			Hash = HashCombine(Hash, GetTypeHash(Vertex.UV));
			Hash = HashCombine(Hash, GetTypeHash(Vertex.Color));
			Hash = HashCombine(Hash, GetTypeHash(Vertex.Tangent));
			return Hash;
		}

		friend FArchive& operator<<(FArchive& Ar, MeshVertex& InVertex)
		{
			Ar << InVertex.Position;
			Ar << InVertex.Normal;
			Ar << InVertex.UV;
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
