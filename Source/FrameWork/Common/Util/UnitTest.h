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
			if (TestModules.Contains(TestName)) {
				TestModules[TestName]();
			}
			else {
				TEST_LOG(LogInit, Display, TEXT("Not found UnitTest Module: %s"), *TestName.ToString());
			}
		}
	private:
		static inline TMap<FName, TFunction<void(void)>> TestModules;
	};
}

#endif