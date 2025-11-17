#pragma once
#include "SpirvDebugger.h"

namespace FW
{
	struct SpvPixelDebuggerContext : SpvDebuggerContext
	{
		SpvPixelDebuggerContext(const Vector2u& InCoord, const TArray<ShaderFunc>& InEditorFuncInfo, const TArray<SpvBinding>& InBindings)
		: SpvDebuggerContext{.EditorFuncInfo = InEditorFuncInfo, .Bindings = InBindings}
		, PixelCoord(InCoord)
		{}

		Vector2u PixelCoord;
	};

	class FRAMEWORK_API SpvPixelDebuggerVisitor : public SpvDebuggerVisitor
	{
	public:
		SpvPixelDebuggerVisitor(SpvPixelDebuggerContext& InPixelContext, bool InEnableUbsan);

	public:
		void Visit(const SpvOpKill* Inst) override;

	protected:
		void PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) override;
	};
}
