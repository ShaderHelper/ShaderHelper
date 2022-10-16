#include "CommonHeader.h"
#if UNIT_TEST

DECLARE_LOG_CATEGORY_EXTERN(LogTestAuxiliary, Log, All);
DEFINE_LOG_CATEGORY(LogTestAuxiliary);
#include <chrono>

namespace SH {
	namespace TEST {
		template<int N>
		struct UnitTmp {
			int Val = 0;
			void Run() {
				TEST_LOG(LogTestAuxiliary, Display, TEXT("TestRunCaseWithInt: %d"), N + Val);
			}
		};

		template<int N>
		struct Functor {
			void operator()() {
				auto t = UnitTmp<N>();
				t.Val = 1;
				t.Run();
			}
		};

		
		template<typename T, T... Seq>
		void TestSeq(TIntegerSequence<T, Seq...>) {
			auto lam = [](T t) {
				TEST_LOG(LogTestAuxiliary, Display, TEXT("TestSeq: %d"), t);
			};
			(lam(Seq), ...);
		}

		void TestAuxiliary()
		{
			TEST_LOG(LogTestAuxiliary, Display, TEXT("Unit Test - Auxiliary:"));
			//Implicit conversion between FVector and Vector.
			{
				FVector fvec(1, 2, 3);
				Vector vec = fvec;
				FVector fvec2 = vec;
				TEST_LOG(LogTestAuxiliary, Display, TEXT("Implicit conversion between FVector and Vector: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
				//Test compatibility with ue math interfaces.
				{
					FTranslationMatrix tranM(vec);
					TEST_LOG(LogTestAuxiliary, Display, TEXT("Test compatibility : (%s)."), *tranM.ToString());
					vec = FMath::VRand();
					TEST_LOG(LogTestAuxiliary, Display, TEXT("Test compatibility : (%s)."), *vec.ToString());
				}
			}

			//Test arithmetic operator
			{
				Vector vec1 = { 1,2,3 };
				Vector vec2 = { 1,1.1,1 };
				{
					Vector vec = vec1 + vec2;
					vec += 2;
					TEST_LOG(LogTestAuxiliary, Display, TEXT("+: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
				}

				{
					Vector vec = vec1 * vec2;
					vec = vec * 2.0;
					vec *= 2;
					TEST_LOG(LogTestAuxiliary, Display, TEXT("*: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
				}
				{
					Vector vec1 = { 1,1,1 };
					(vec1 += 1) = { 3,3,3 };
					TEST_LOG(LogTestAuxiliary, Display, TEXT("+=: (%s)."), *vec1.ToString());
				}
			}

			//Test Swizzle
			{
				Vector vec1 = { 1,2,3 };
				Vector vec2 = vec1.XXY;
				TEST_LOG(LogTestAuxiliary, Display, TEXT("Swizzle: (%lf,%lf,%lf)."), vec2.X, vec2.Y,vec2.Z);
				{
					Vector2D vec3 = vec2.XZ + 3.0;
					TEST_LOG(LogTestAuxiliary, Display, TEXT("Swizzle +: (%lf,%lf)."), vec3.X, vec3.Y);
				}
			
				{
					Vector4 vec4 = 2.0 * vec2.ZZZZ; // 2 * {2,2,2,2}
					TEST_LOG(LogTestAuxiliary, Display, TEXT("Swizzle *: (%lf,%lf,%lf,%lf)."), vec4.X, vec4.Y, vec4.Z, vec4.W);
				}

				{
					Vector vec4 = vec1 * vec2.ZZZ; //{1,2,3} * {2,2,2}
					TEST_LOG(LogTestAuxiliary, Display, TEXT("Swizzle *: (%lf,%lf,%lf)."), vec4.X, vec4.Y, vec4.Z);
				}

				{
					Vector2D vec4 = vec1.ZZ / vec2.XZ; // {3,3} / {1,2}
					TEST_LOG(LogTestAuxiliary, Display, TEXT("Swizzle /: (%lf,%lf)."), vec4.X, vec4.Y);
				}
			}

			//Test OutPtr
			{
				struct Unit { 
					Unit(int v) : Value(v) {}
					~Unit() { TEST_LOG(LogTestAuxiliary, Display, TEXT("~Unit:%d"),Value); } 
					int Value;
				};
				TSharedPtr<Unit> t = MakeShared<Unit>(114);
				
				auto testf = [](Unit** p) { *p = new Unit(514); };
				testf(AUX::OutPtr(t));

				auto testf2 = [](void** p) {*p = new Unit(1919); };
				testf2(AUX::OutPtr<void*>(t));
			}

			//Test Seq
			{
				TestSeq(AUX::MakeRangeIntegerSequence<uint32, 0, 2>{});
				//Avoid the following code: 
				//if (VarFromFile == 2) {
				//	UnitTmp<2>() ...
				//}
				//else if (VarFromFile == 3) {
				//	UnitTmp<3>() ...
				//}...
				int VarFromFile = 60;
				AUX::RunCaseWithInt<2, 64, Functor>(VarFromFile);
			}

		}
	}
}
#endif