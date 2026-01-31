#pragma once
#include "Editor/Editor.h"
#include "TestViewport.h"

namespace UNITTEST_GPUAPI
{

	class UnitTestEditor : public FW::Editor
	{
	public:
		UnitTestEditor(const FW::Vector2f& InWindowSize);
		void AddTest(const FString& InName, const TFunction<void(TestViewport*)>InTest, FW::GpuTextureFormat ViewportFormat = FW::GpuTextureFormat::B8G8R8A8_UNORM);

	private:
		FW::Vector2f WindowSize;
	};

}


