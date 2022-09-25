#pragma once
#include "CommonHeader.h"
#if UNIT_TEST

namespace FRAMEWORK {
	class UnitTest {
	public:
		static void Register(const FName& ModuleName, TFunction<void(void)> TestFunc) {
			TestModules.Add(ModuleName, TestFunc);
		}
		static void Run(const FName& TestName) {
			TestModules[TestName]();
		}
	private:
		static inline TMap<FName, TFunction<void(void)>> TestModules;
	};
}

#endif