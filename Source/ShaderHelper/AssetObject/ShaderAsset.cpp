#include "CommonHeader.h"
#include "ShaderAsset.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderAsset>("ShaderAsset")
								.BaseClass<AssetObject>()
	)

	ShaderAsset::ShaderAsset()
	{

	}

	void ShaderAsset::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		
		Ar << EditorContent;
		SavedEditorContent = EditorContent;
	}
}
