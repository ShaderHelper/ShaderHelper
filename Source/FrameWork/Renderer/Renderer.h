#pragma once
#include "CommonHeader.h"
namespace FRAMEWORK
{
	class Renderer : public FNoncopyable
	{
	public:
		Renderer() = default;
		virtual ~Renderer() = default;
	public:
		virtual void* GetSharedHanldeFromFinalRT() const { return nullptr; };
		virtual void Render() = 0;
	};
}


