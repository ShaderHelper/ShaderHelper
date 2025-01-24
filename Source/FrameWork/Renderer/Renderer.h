#pragma once

namespace FW
{
	class FRAMEWORK_API Renderer : public FNoncopyable
	{
	public:
		Renderer() = default;
		virtual ~Renderer() = default;
        
	public:
		virtual void Render() = 0;
	};
}


