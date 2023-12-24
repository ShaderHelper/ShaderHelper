#pragma once
#include "Editor/Editor.h"

namespace UNITTEST_UTIL 
{

	class UnitTestEditor : public Editor
	{
	public:
		UnitTestEditor(const Vector2f& InWindowSize);
		virtual ~UnitTestEditor();

	private:
		void InitLogWindow();

	private:
		Vector2f WindowSize;
	};

}


