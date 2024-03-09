#pragma once
#include "Editor/Editor.h"

namespace UNITTEST_GPUAPI
{

	class UnitTestEditor : public FRAMEWORK::Editor
	{
	public:
		UnitTestEditor(const FRAMEWORK::Vector2f& InWindowSize);
		virtual ~UnitTestEditor();

	private:
        FRAMEWORK::Vector2f WindowSize;
	};

}


