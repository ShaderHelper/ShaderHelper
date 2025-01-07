#pragma once

namespace FW
{
	class FRAMEWORK_API RenderComponent
	{
	public:
		RenderComponent() = default;
		virtual ~RenderComponent() = default;

	public:
		virtual void RenderBegin() {}
		virtual void RenderInternal() = 0;
		virtual void RenderEnd() {}
	};
}