#pragma once
#include "AssetObject.h"
#include "RenderResource/Mesh.h"

namespace FW
{
	class FRAMEWORK_API Model : public AssetObject
	{
		REFLECTION_TYPE(Model)
	public:
		Model();
		Model(TArray<MeshData> InSubMeshes);

	public:
		void Serialize(FArchive& Ar) override;
		void PostLoad() override;

		void InitGpuData();
		FString FileExtension() const override;
		const TArray<MeshData>& GetSubMeshes() const { return SubMeshes; }
		const TArray<MeshBuffers>& GetGpuMeshes() const { return GpuMeshes; }

		TRefCountPtr<GpuTexture> GenerateThumbnail() const override;

		void ComputeBounds(Vector3f& OutCenter, float& OutRadius) const;

	public:
		uint32 VertexCount = 0;
		uint32 TriangleFaceCount = 0;

	private:
		TArray<MeshData> SubMeshes;
		TArray<MeshBuffers> GpuMeshes;
	};
}
