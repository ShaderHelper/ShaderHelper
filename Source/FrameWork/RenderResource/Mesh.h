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
	};

	struct MeshData
	{
		TArray<MeshVertex> Vertices;
		TArray<uint32> Indices;
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
