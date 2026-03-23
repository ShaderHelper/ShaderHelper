#pragma once
#include "Editor/Editor.h"
#include "TestViewport.h"

namespace UNITTEST_GPUAPI
{
	class UnitTestEditor : public FW::Editor
	{
	public:
		UnitTestEditor(const FW::Vector2f& InWindowSize);
		void AddTest(TUniquePtr<class TestUnit> InTestUnit);

	private:
		FW::Vector2f WindowSize;
	};

}


