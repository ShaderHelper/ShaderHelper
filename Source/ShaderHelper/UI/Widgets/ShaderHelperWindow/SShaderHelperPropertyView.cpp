#include "CommonHeader.h"
#include "SShaderHelperPropertyView.h"

namespace SH
{

	void SShaderHelperPropertyView::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SAssignNew(PropertyTree, SPropertyView<PropertyDataType>)
			.PropertyDatas(&PropertyDatas)
		];
	}

}