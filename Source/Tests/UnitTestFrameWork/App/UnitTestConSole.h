#pragma once
#include "FrameWork/App/App.h"

class UnitTestConSole : public App
{
public:
	UnitTestConSole(const TCHAR* CommandLine);
private:
	void Init() override;
	void ShutDown() override;
	void PostInit() override;
	void Update(double DeltaTime) override;

	void InitLogWindow();
};
	


