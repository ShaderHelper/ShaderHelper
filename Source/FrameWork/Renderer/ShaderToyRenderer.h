#pragma once
#include "Renderer/Renderer.h"

namespace SH
{
	class ShaderToyRenderer : public FW::Renderer
	{
	public:

	public:
		void OnViewportResize(const FW::Vector2f& InResolution);
	};
}