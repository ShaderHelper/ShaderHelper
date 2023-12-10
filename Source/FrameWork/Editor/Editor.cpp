#include "CommonHeader.h"
#include "Editor.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FRAMEWORK 
{

	Editor::Editor()
	{
		FAppCommonStyle::Init();
	}

	Editor::~Editor()
	{
		FAppCommonStyle::ShutDown();
	}

}
