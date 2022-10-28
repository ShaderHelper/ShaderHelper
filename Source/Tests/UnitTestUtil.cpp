#include "CommonHeader.h"
#if UNIT_TEST

DECLARE_LOG_CATEGORY_EXTERN(LogTestUtil, Log, All);
DEFINE_LOG_CATEGORY(LogTestUtil);
#include <chrono>

namespace SH {
	namespace TEST {
		template<int N,typename T = int>
		struct UnitTmp {
			T Val = 0;
			void Run(const FString& ss) {
				TEST_LOG(LogTestUtil, Display, TEXT("TestRunCaseWithInt: %d"), N + Val);
				TEST_LOG(LogTestUtil, Display, TEXT("TestRunCaseWithInt: %s"), *ss);
			}
		};
		template<int N>
		void UnitTmp2() {
			TEST_LOG(LogTestUtil, Display, TEXT("TestRunCaseWithInt: %d"), N);
		}

		template<int N, typename T>
		struct Functor {
			int operator()(FString&& ss, int vv) {
				UnitTmp2<N>();
				auto t = UnitTmp<N,T>();
				t.Val = 1 + vv;
				t.Run(ss);
				return t.Val;
			}
		};

		
		template<typename T, T... Seq>
		void TestSeq(TIntegerSequence<T, Seq...>) {
			auto lam = [](T t) {
				TEST_LOG(LogTestUtil, Display, TEXT("TestSeq: %d"), t);
			};
			(lam(Seq), ...);
		}

		void TestUtil()
		{
			TEST_LOG(LogTestUtil, Display, TEXT("Unit Test - Util:"));
			//Implicit conversion between FVector and Vector.
			{
				FVector fvec(1, 2, 3);
				Vector vec = fvec;
				FVector fvec2 = vec;
				TEST_LOG(LogTestUtil, Display, TEXT("Implicit conversion between FVector and Vector: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
				//Test compatibility with ue math interfaces.
				{
					FTranslationMatrix tranM(vec);
					TEST_LOG(LogTestUtil, Display, TEXT("Test compatibility : (%s)."), *tranM.ToString());
					vec = FMath::VRand();
					TEST_LOG(LogTestUtil, Display, TEXT("Test compatibility : (%s)."), *vec.ToString());
				}
			}

			//Test arithmetic operator
			{
				Vector vec1 = { 1,2,3 };
				Vector vec2 = { 1,1.1,1 };
				{
					Vector vec = vec1 + vec2;
					vec += 2;
					TEST_LOG(LogTestUtil, Display, TEXT("+: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
				}

				{
					Vector vec = vec1 * vec2;
					vec = vec * 2.0;
					vec *= 2;
					TEST_LOG(LogTestUtil, Display, TEXT("*: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
				}
				{
					Vector vec1 = { 1,1,1 };
					(vec1 += 1) = { 3,3,3 };
					TEST_LOG(LogTestUtil, Display, TEXT("+=: (%s)."), *vec1.ToString());
				}
			}

			//Test Swizzle
			{
				Vector vec1 = { 1,2,3 };
				Vector vec2 = vec1.XXY;
				TEST_LOG(LogTestUtil, Display, TEXT("Swizzle: (%lf,%lf,%lf)."), vec2.X, vec2.Y,vec2.Z);
				{
					Vector2D vec3 = vec2.XZ + 3.0;
					TEST_LOG(LogTestUtil, Display, TEXT("Swizzle +: (%lf,%lf)."), vec3.X, vec3.Y);
				}
			
				{
					Vector4 vec4 = 2.0 * vec2.ZZZZ; // 2 * {2,2,2,2}
					TEST_LOG(LogTestUtil, Display, TEXT("Swizzle *: (%lf,%lf,%lf,%lf)."), vec4.X, vec4.Y, vec4.Z, vec4.W);
				}

				{
					Vector vec4 = vec1 * vec2.ZZZ; //{1,2,3} * {2,2,2}
					TEST_LOG(LogTestUtil, Display, TEXT("Swizzle *: (%lf,%lf,%lf)."), vec4.X, vec4.Y, vec4.Z);
				}

				{
					Vector2D vec4 = vec1.ZZ / vec2.XZ; // {3,3} / {1,2}
					TEST_LOG(LogTestUtil, Display, TEXT("Swizzle /: (%lf,%lf)."), vec4.X, vec4.Y);
				}
			}

			//Test OutPtr
			{
				struct Unit { 
					Unit(int v) : Value(v) {}
					~Unit() { TEST_LOG(LogTestUtil, Display, TEXT("~Unit:%d"),Value); } 
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
				// template<typename T>
				// void SomeFunction()
				// {
				// 	if (VarFromFile == 2) {
				//		//Functor execute code:
				//		UnitTmp<2,T>() ...
				//	}
				//	else if (VarFromFile == 3) {
				//		UnitTmp<3,T>() ...
				//	}
				//	.
				//	.
				//	.
				//	else (VarFromFile == 64) {
				//		UnitTmp<64,T>() ...
				//	}
				// }
	
				int VarFromFile = 62;
				int v = AUX::RunCaseWithInt<2, 64, Functor, int /*T*/>(VarFromFile, TEXT("String"), 11);
				TEST_LOG(LogTestUtil, Display, TEXT("TestRunCaseWithInt: %d"), v);
			}
		
			//Test observableUniquePtr
			{
				
			}
		}
	}
}
#endif