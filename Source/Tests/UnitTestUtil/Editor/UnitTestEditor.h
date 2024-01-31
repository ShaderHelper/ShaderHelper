#pragma once
#include "Editor/Editor.h"

namespace UNITTEST_UTIL 
{

	class UnitTestEditor : public FRAMEWORK::Editor
	{
	public:
		UnitTestEditor(const FRAMEWORK::Vector2f& InWindowSize);
		virtual ~UnitTestEditor();

	private:
		void InitLogWindow();

	private:
        FRAMEWORK::Vector2f WindowSize;
	};

}


