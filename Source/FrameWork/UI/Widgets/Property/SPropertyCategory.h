#pragma once

namespace FRAMEWORK
{

	class FRAMEWORK_API SPropertyCatergory : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyCatergory) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	};
}


