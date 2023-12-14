#include "CommonHeader.h"
#include "Texture2D.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuApiInterface.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<Texture2D>()
						.BaseClass<AssetObject>();
	)

	Texture2D::Texture2D()
		: Width(0), Height(0)
	{

	}

	Texture2D::Texture2D(int32 InWidth, int32 InHeight, const TArray<uint8>& InRawData)
		: Width(InWidth)
		, Height(InHeight)
		, RawData(InRawData)
	{
		
	}

	void Texture2D::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << Width;
		Ar << Height;
		Ar << RawData;
	}

	void Texture2D::PostLoad()
	{

	}

	FString Texture2D::FileExtension() const
	{
		return "Texture";
	}

	GpuTexture* Texture2D::Gethumbnail() const
	{
		return nullptr;
	}

	TSharedRef<SWidget> Texture2D::GetPropertyView() const
	{
		return SNullWidget::NullWidget;
	}

}