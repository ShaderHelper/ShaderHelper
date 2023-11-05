#pragma once
#include "ArgumentBuffer.h"

namespace FRAMEWORK
{
	class ShaderMap
	{
		friend class ShaderMapBuilder;
	public:
		bool BeginCompileMap()
		{

		}

	private:
		TRefCountPtr<GpuShader> Vs;
		TRefCountPtr<GpuShader> Ps;

		TSharedPtr<ArgumentBufferLayout> Layouts[GpuResourceLimit::MaxBindableBingGroupNum];
	};

	class ShaderMapBuilder
	{
	public:
		ShaderMapBuilder& SetVertexShader(TRefCountPtr<GpuShader> InVs) {
			Shaders.Vs = MoveTemp(InVs);
			return *this;
		}

		ShaderMapBuilder& SetPixelShader(TRefCountPtr<GpuShader> InPs) {
			Shaders.Ps = MoveTemp(InPs);
			return *this;
		}

		template<uint32 Slot>
		ShaderMapBuilder& SetArgumentBufferLayout(TSharedPtr<ArgumentBufferLayout> InLayout) {
			static_assert(Slot < GpuResourceLimit::MaxBindableBingGroupNum, "Slot exceeded safe binding range");
			Shaders.Layouts[Slot] = MoveTemp(InLayout);
			return *this;
		}

		operator TUniquePtr<ShaderMap>() const {
			return MakeUnique<ShaderMap>(Shaders);
		}

	private:
		ShaderMap Shaders;
	};

}