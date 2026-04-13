#include "CommonHeader.h"
#include "Mesh.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	static Vector4f ComputeFaceTangent(
		const Vector3f& Normal,
		const Vector3f& P0, const Vector3f& P1, const Vector3f& P2,
		const Vector2f& UV0, const Vector2f& UV1, const Vector2f& UV2)
	{
		const Vector3f Edge1 = P1 - P0;
		const Vector3f Edge2 = P2 - P0;
		const float DeltaU1 = UV1.X - UV0.X;
		const float DeltaV1 = UV1.Y - UV0.Y;
		const float DeltaU2 = UV2.X - UV0.X;
		const float DeltaV2 = UV2.Y - UV0.Y;

		const float Det = DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1;
		if (FMath::IsNearlyZero(Det))
		{
			return Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
		}
		const float InvDet = 1.0f / Det;

		Vector3f T = (Edge1 * DeltaV2 - Edge2 * DeltaV1) * InvDet;
		T = FVector3f(T).GetSafeNormal();

		const Vector3f B = (Edge2 * DeltaU1 - Edge1 * DeltaU2) * InvDet;
		const float Sign = FVector3f::DotProduct(FVector3f::CrossProduct(FVector3f(Normal), FVector3f(T)), FVector3f(B)) >= 0.0f ? 1.0f : -1.0f;

		return Vector4f(T.X, T.Y, T.Z, Sign);
	}

	static void AppendFace(
		MeshData& OutMeshData,
		const Vector3f& Normal,
		const Vector3f& P0,
		const Vector3f& P1,
		const Vector3f& P2,
		const Vector3f& P3,
		const Vector2f& UV0,
		const Vector2f& UV1,
		const Vector2f& UV2,
		const Vector2f& UV3)
	{
		const Vector4f Tangent = ComputeFaceTangent(Normal, P0, P1, P2, UV0, UV1, UV2);
		const uint32 BaseVertex = OutMeshData.Vertices.Num();
		OutMeshData.Vertices.Append({
			{ P0, Normal, UV0, Vector4f(1.0f), Tangent },
			{ P1, Normal, UV1, Vector4f(1.0f), Tangent },
			{ P2, Normal, UV2, Vector4f(1.0f), Tangent },
			{ P3, Normal, UV3, Vector4f(1.0f), Tangent },
		});
		OutMeshData.Indices.Append({
			BaseVertex + 0, BaseVertex + 1, BaseVertex + 2,
			BaseVertex + 0, BaseVertex + 2, BaseVertex + 3,
		});
	}

	static void AppendFace(MeshData& OutMeshData, const Vector3f& Normal, const Vector3f& P0, const Vector3f& P1, const Vector3f& P2, const Vector3f& P3)
	{
		AppendFace(
			OutMeshData,
			Normal,
			P0,
			P1,
			P2,
			P3,
			Vector2f{ 0.0f, 0.0f },
			Vector2f{ 1.0f, 0.0f },
			Vector2f{ 1.0f, 1.0f },
			Vector2f{ 0.0f, 1.0f }
		);
	}

	MeshData CreateQuad()
	{
		MeshData OutMeshData;
		AppendFace(OutMeshData, Vector3f{ 0.0f, 0.0f, -1.0f },
			Vector3f{ -0.5f, 0.5f, 0.0f }, Vector3f{ 0.5f, 0.5f, 0.0f }, Vector3f{ 0.5f, -0.5f, 0.0f }, Vector3f{ -0.5f, -0.5f, 0.0f });
		return OutMeshData;
	}

	MeshData CreateCube()
	{
		MeshData OutMeshData;
		AppendFace(
			OutMeshData,
			Vector3f{ 0.0f, 0.0f, 1.0f },
			Vector3f{ -0.5f, -0.5f, 0.5f }, Vector3f{ 0.5f, -0.5f, 0.5f }, Vector3f{ 0.5f, 0.5f, 0.5f }, Vector3f{ -0.5f, 0.5f, 0.5f },
			Vector2f{ 1.0f, 1.0f }, Vector2f{ 0.0f, 1.0f }, Vector2f{ 0.0f, 0.0f }, Vector2f{ 1.0f, 0.0f }
		);
		AppendFace(
			OutMeshData,
			Vector3f{ 0.0f, 0.0f, -1.0f },
			Vector3f{ 0.5f, -0.5f, -0.5f }, Vector3f{ -0.5f, -0.5f, -0.5f }, Vector3f{ -0.5f, 0.5f, -0.5f }, Vector3f{ 0.5f, 0.5f, -0.5f },
			Vector2f{ 1.0f, 1.0f }, Vector2f{ 0.0f, 1.0f }, Vector2f{ 0.0f, 0.0f }, Vector2f{ 1.0f, 0.0f }
		);
		AppendFace(
			OutMeshData,
			Vector3f{ -1.0f, 0.0f, 0.0f },
			Vector3f{ -0.5f, -0.5f, -0.5f }, Vector3f{ -0.5f, -0.5f, 0.5f }, Vector3f{ -0.5f, 0.5f, 0.5f }, Vector3f{ -0.5f, 0.5f, -0.5f },
			Vector2f{ 1.0f, 1.0f }, Vector2f{ 0.0f, 1.0f }, Vector2f{ 0.0f, 0.0f }, Vector2f{ 1.0f, 0.0f }
		);
		AppendFace(
			OutMeshData,
			Vector3f{ 1.0f, 0.0f, 0.0f },
			Vector3f{ 0.5f, -0.5f, 0.5f }, Vector3f{ 0.5f, -0.5f, -0.5f }, Vector3f{ 0.5f, 0.5f, -0.5f }, Vector3f{ 0.5f, 0.5f, 0.5f },
			Vector2f{ 1.0f, 1.0f }, Vector2f{ 0.0f, 1.0f }, Vector2f{ 0.0f, 0.0f }, Vector2f{ 1.0f, 0.0f }
		);
		AppendFace(
			OutMeshData,
			Vector3f{ 0.0f, 1.0f, 0.0f },
			Vector3f{ -0.5f, 0.5f, 0.5f }, Vector3f{ 0.5f, 0.5f, 0.5f }, Vector3f{ 0.5f, 0.5f, -0.5f }, Vector3f{ -0.5f, 0.5f, -0.5f },
			Vector2f{ 0.0f, 0.0f }, Vector2f{ 1.0f, 0.0f }, Vector2f{ 1.0f, 1.0f }, Vector2f{ 0.0f, 1.0f }
		);
		AppendFace(
			OutMeshData,
			Vector3f{ 0.0f, -1.0f, 0.0f },
			Vector3f{ -0.5f, -0.5f, -0.5f }, Vector3f{ 0.5f, -0.5f, -0.5f }, Vector3f{ 0.5f, -0.5f, 0.5f }, Vector3f{ -0.5f, -0.5f, 0.5f },
			Vector2f{ 1.0f, 1.0f }, Vector2f{ 0.0f, 1.0f }, Vector2f{ 0.0f, 0.0f }, Vector2f{ 1.0f, 0.0f }
		);
		return OutMeshData;
	}

	MeshData CreateSphere(uint32 SliceCount, uint32 StackCount)
	{
		MeshData OutMeshData;
		SliceCount = FMath::Max(SliceCount, 3u);
		StackCount = FMath::Max(StackCount, 2u);

		for (uint32 Stack = 0; Stack <= StackCount; ++Stack)
		{
			const float V = (float)Stack / (float)StackCount;
			const float Phi = PI * V;
			const float SinPhi = FMath::Sin(Phi);
			const float CosPhi = FMath::Cos(Phi);

			for (uint32 Slice = 0; Slice <= SliceCount; ++Slice)
			{
				const float U = (float)Slice / (float)SliceCount;
				const float Theta = 2.0f * PI * U;
				const float SinTheta = FMath::Sin(Theta);
				const float CosTheta = FMath::Cos(Theta);
				const Vector3f Normal{ SinPhi * CosTheta, CosPhi, SinPhi * SinTheta };
				// dP/dTheta direction, normalized: (-SinTheta, 0, CosTheta). At poles SinPhi≈0, fallback is fine since the direction is still valid.
				const Vector3f T{ -SinTheta, 0.0f, CosTheta };
				OutMeshData.Vertices.Add({ Normal * 0.5f, Normal, Vector2f{ U, V }, Vector4f(1.0f), Vector4f(T.X, T.Y, T.Z, 1.0f) });
			}
		}

		const uint32 RingVertexCount = SliceCount + 1;
		for (uint32 Stack = 0; Stack < StackCount; ++Stack)
		{
			for (uint32 Slice = 0; Slice < SliceCount; ++Slice)
			{
				const uint32 I0 = Stack * RingVertexCount + Slice;
				const uint32 I1 = I0 + 1;
				const uint32 I2 = I0 + RingVertexCount;
				const uint32 I3 = I2 + 1;
				OutMeshData.Indices.Append({ I0, I1, I2, I1, I3, I2 });
			}
		}

		return OutMeshData;
	}

	MeshBuffers UploadMesh(const MeshData& InMeshData)
	{
		TArray<uint8> VertexBytes(reinterpret_cast<const uint8*>(InMeshData.Vertices.GetData()), InMeshData.Vertices.Num() * sizeof(MeshVertex));
		TArray<uint8> IndexBytes(reinterpret_cast<const uint8*>(InMeshData.Indices.GetData()), InMeshData.Indices.Num() * sizeof(uint32));

		MeshBuffers Buffers;
		Buffers.VertexCount = InMeshData.Vertices.Num();
		Buffers.IndexCount = InMeshData.Indices.Num();
		Buffers.VertexBuffer = GGpuRhi->CreateBuffer({
			.ByteSize = static_cast<uint32>(VertexBytes.Num()),
			.Usage = GpuBufferUsage::Vertex,
			.InitialData = VertexBytes,
		});
		Buffers.IndexBuffer = GGpuRhi->CreateBuffer({
			.ByteSize = static_cast<uint32>(IndexBytes.Num()),
			.Usage = GpuBufferUsage::Index,
			.InitialData = IndexBytes,
		});
		return Buffers;
	}
}
