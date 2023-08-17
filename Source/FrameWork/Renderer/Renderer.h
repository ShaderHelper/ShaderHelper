#pragma once

namespace FRAMEWORK
{
	class FRAMEWORK_API Renderer : public FNoncopyable
	{
	public:
		Renderer();
		virtual ~Renderer() = default;
        
	public:
        void Render();
        
    protected:
        virtual void RenderInternal() {};
        virtual void RenderBegin();
        virtual void RenderEnd();
	};
}


