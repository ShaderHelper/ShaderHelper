#pragma once
#include "CommonHeader.h"
#if UNIT_TEST

namespace SH {
	namespace TEST {
		extern void TestAuxiliary();

		inline void UnitTest(const FName& TestName) {
			if (TestName == FName("Auxiliary")) {
				TestAuxiliary();
			}
		}
	}
}

#endif