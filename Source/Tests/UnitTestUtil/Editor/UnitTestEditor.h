#pragma once
#include "Editor/Editor.h"

namespace UNITTEST_UTIL 
{

	class UnitTestEditor : public FW::Editor
	{
	public:
		UnitTestEditor(const FW::Vector2f& InWindowSize);
		virtual ~UnitTestEditor();

	private:
		void InitLogWindow();

	private:
        FW::Vector2f WindowSize;
	};

}


