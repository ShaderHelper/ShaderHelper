#pragma once
#include "Editor/Editor.h"

namespace UNITTEST_GPUAPI
{

	class UnitTestEditor : public Editor
	{
	public:
		UnitTestEditor(const Vector2f& InWindowSize);
		virtual ~UnitTestEditor();

	private:
		Vector2f WindowSize;
	};

}


