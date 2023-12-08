#include "CommonHeader.h"
#include "Texture2D.h"
#include "Common/Util/Reflection.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<Texture2D>()
						.BaseClass<AssetObject>();
	)

	void Texture2D::Serialize(FArchive& Ar)
	{

	}

	void Texture2D::PostLoad()
	{

	}

	FString Texture2D::FileExtension() const
	{
		return "Texture";
	}

}