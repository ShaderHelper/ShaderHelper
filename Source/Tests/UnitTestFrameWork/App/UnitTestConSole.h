#pragma once
#include "FrameWork/App/App.h"

class UnitTestConSole : public App
{
public:
	UnitTestConSole(const TCHAR* CommandLine);
public:

	void Init();
	void ShutDown();
	void PostInit();
	void Update(double DeltaTime);
};
	


