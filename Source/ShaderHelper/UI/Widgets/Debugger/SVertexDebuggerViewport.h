#pragma once

namespace SH
{
	class SVertexDebuggerViewport : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SVertexDebuggerViewport){}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
	};
}
