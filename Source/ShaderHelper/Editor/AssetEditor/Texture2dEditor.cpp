#include "CommonHeader.h"
#include "Texture2dEditor.h"
#include "AssetObject/Texture2D.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<Texture2dOp>()
		.BaseClass<AssetOp>()
	)

	MetaType* Texture2dOp::SupportType()
	{
		return GetMetaType<Texture2D>();
	}

}
