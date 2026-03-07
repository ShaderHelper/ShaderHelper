#include "CommonHeader.h"
#include "TextureCube.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	REFLECTION_REGISTER(AddClass<TextureCube>("TextureCube")
		.BaseClass<AssetObject>()
	)

	TextureCube::TextureCube()
		: Size(0), Format(GpuTextureFormat::B8G8R8A8_UNORM)
	{
		FaceData.SetNum(6);
	}

	TextureCube::TextureCube(uint32 InSize, GpuTextureFormat InFormat, const TArray<TArray<uint8>>& InFaceData)
		: Size(InSize)
		, Format(InFormat)
		, FaceData(InFaceData)
	{
		check(FaceData.Num() == 6);
		InitGpuData();
	}

	void TextureCube::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		Ar << Format;
		Ar << Size;
		Ar << FaceData;
	}

	void TextureCube::PostLoad()
	{
		AssetObject::PostLoad();
		InitGpuData();
	}

	void TextureCube::InitGpuData()
	{
		if (Size == 0) return;

		GpuTextureDesc Desc;
		Desc.Width = Size;
		Desc.Height = Size;
		Desc.Format = Format;
		Desc.Usage = GpuTextureUsage::ShaderResource;
		Desc.Dimension = GpuTextureDimension::TexCube;

		GpuData = GGpuRhi->CreateTexture(Desc, GpuResourceState::CopyDst);

		GpuCmdRecorder* CmdRecorder = GGpuRhi->BeginRecording(TEXT("InitCubeMap"));
		{
			const uint32 FaceByteSize = Size * Size * GetTextureFormatByteSize(Format);
			for (int32 Face = 0; Face < 6; Face++)
			{
				if (FaceData[Face].Num() == 0) continue;

				TRefCountPtr<GpuBuffer> UploadBuffer = GGpuRhi->CreateBuffer(
					{FaceByteSize, GpuBufferUsage::Upload}, GpuResourceState::CopySrc
				);
				void* Mapped = GGpuRhi->MapGpuBuffer(UploadBuffer, GpuResourceMapMode::Write_Only);
				FMemory::Memcpy(Mapped, FaceData[Face].GetData(), FaceByteSize);
				GGpuRhi->UnMapGpuBuffer(UploadBuffer);

				CmdRecorder->CopyBufferToTexture(UploadBuffer, GpuData, Face, 0);
			}

			CmdRecorder->Barriers({{GpuData, GpuResourceState::ShaderResourceRead}});
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({CmdRecorder});

		// Create a 2D shared texture from face 0 for preview/thumbnail
		if (FaceData.Num() == 6 && FaceData[0].Num() > 0)
		{
			GpuTextureDesc PreviewDesc{ Size, Size, Format, GpuTextureUsage::ShaderResource | GpuTextureUsage::Shared, FaceData[0] };
			PreviewData = GGpuRhi->CreateTexture(MoveTemp(PreviewDesc));
		}
	}

	FString TextureCube::FileExtension() const
	{
		return TEXT("cubemap");
	}

	GpuTexture* TextureCube::GetThumbnail() const
	{
		if (PreviewData.IsValid())
		{
			return PreviewData;
		}
		return nullptr;
	}

	void TextureCube::SetFaceData(int32 FaceIndex, const TArray<uint8>& InData)
	{
		check(FaceIndex >= 0 && FaceIndex < 6);
		FaceData[FaceIndex] = InData;
	}

	void TextureCube::SetData(uint32 InSize, GpuTextureFormat InFormat, TArray<TArray<uint8>> InFaceData)
	{
		check(InFaceData.Num() == 6);
		Size = InSize;
		Format = InFormat;
		FaceData = MoveTemp(InFaceData);
		InitGpuData();
	}
}
