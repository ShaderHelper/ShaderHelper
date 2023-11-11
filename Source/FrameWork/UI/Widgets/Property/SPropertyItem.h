#pragma once

namespace FRAMEWORK
{

	class FRAMEWORK_API SPropertyItem : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyItem) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	};
}


