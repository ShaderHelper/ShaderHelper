#include "CommonHeader.h"
#include "UnitTestEditor.h"

using namespace FW;

namespace UNITTEST_GPUAPI
{

	UnitTestEditor::UnitTestEditor(const FW::Vector2f& InWindowSize)
		: WindowSize(InWindowSize)
	{
		FSlateApplication::Get().AddWindow(SNew(SWindow));
	}

	UnitTestEditor::~UnitTestEditor()
	{
		
	}


}
