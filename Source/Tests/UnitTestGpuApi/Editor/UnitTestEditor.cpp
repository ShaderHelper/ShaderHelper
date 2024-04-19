#include "CommonHeader.h"
#include "UnitTestEditor.h"

using namespace FRAMEWORK;

namespace UNITTEST_GPUAPI
{

	UnitTestEditor::UnitTestEditor(const FRAMEWORK::Vector2f& InWindowSize)
		: WindowSize(InWindowSize)
	{
		FSlateApplication::Get().AddWindow(SNew(SWindow));
	}

	UnitTestEditor::~UnitTestEditor()
	{
		
	}


}
