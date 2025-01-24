#pragma once
#include "Editor/Editor.h"

namespace UNITTEST_GPUAPI
{

	class UnitTestEditor : public FW::Editor
	{
	public:
		UnitTestEditor(const FW::Vector2f& InWindowSize);
		virtual ~UnitTestEditor();

	private:
        FW::Vector2f WindowSize;
	};

}


