#include "CommonHeader.h"
#include "magic_enum.hpp"
DEFINE_LOG_CATEGORY_STATIC(LogTestUtil, Log, All);
#include <chrono>

using namespace FW;

namespace UNITTEST_UTIL
{
	enum class EnumClassTest
	{
		A,
		B,
		Num
	};

	template<int N, int M = 0>
	struct UnitTmp {
		void Run(const FString& ss) {
			SH_LOG(LogTestUtil, Display, TEXT("Test RUNTIME_INTEGER_CONV: %s"), *ss);
		}

		double Result = N + M;
	};

	template<EnumClassTest E>
	struct UnitTmp2 {
		void Run() {
			SH_LOG(LogTestUtil, Display, TEXT("Test RUNTIME_INTEGER_CONV: %s"), ANSI_TO_TCHAR(magic_enum::enum_name(E).data()));
		}
	};

	template<typename T, T... Seq>
	void TestSeq(TIntegerSequence<T, Seq...>) {
		SH_LOG(LogTestUtil, Display, TEXT("TestSeq: %s"), *(FString::Printf(TEXT("%d, "), Seq) + ...));
	}

	void TestFunctorToFuncPtr(void(*func)(int,int))
	{
		func(1,2);
	}

	struct PrivateUnitTestBase
	{
	protected:
		virtual void Run() {
			SH_LOG(LogTestUtil, Display, TEXT("PrivateUnitTestBase::Run"));
		}
	};

    struct PrivateUnitTest : public PrivateUnitTestBase
    {
        int GetVar() const {return Var;}
	protected:
		virtual void Run() override {
			SH_LOG(LogTestUtil, Display, TEXT("PrivateUnitTest::Run"));
		}
	private:
		void test(int a) && {
			SH_LOG(LogTestUtil, Display, TEXT("Test CALL_PRIVATE_FUNCTION:%d"), a);
		}
        void overload(int a, int b) {
            SH_LOG(LogTestUtil, Display, TEXT("Test CALL_PRIVATE_FUNCTION:%d"), a + b);
        }
        void overload(float a) {
            SH_LOG(LogTestUtil, Display, TEXT("Test CALL_PRIVATE_FUNCTION:%f"), a);
        }
        int Var = 233;
    };
    
    STEAL_PRIVATE_MEMBER(PrivateUnitTest, int, Var)
	CALL_PRIVATE_FUNCTION(PrivateUnitTest_test, PrivateUnitTest, test, &&, void, int)
    CALL_PRIVATE_FUNCTION(PrivateUnitTest_overload1, PrivateUnitTest, overload,, void, int, int)
    CALL_PRIVATE_FUNCTION(PrivateUnitTest_overload2, PrivateUnitTest, overload,, void, float)
	CALL_PRIVATE_FUNCTION(PrivateUnitTestBase_Run, PrivateUnitTestBase, Run,, void)

	void TestUtil()
	{
		SH_LOG(LogTestUtil, Display, TEXT("Unit Test - Util:"));
		//Implicit conversion between FVector and Vector.
		{
			FVector fvec(1, 2, 3);
			Vector vec = fvec;
			FVector3f fvec2 = vec;
			SH_LOG(LogTestUtil, Display, TEXT("Implicit conversion between FVector and Vector: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);

			Vector vv{};// {0,0,0}
			SH_LOG(LogTestUtil, Display, TEXT("Vector Value Init: (%s)."), *vv.ToString());
            
            Vector3d vecx{1.0f};
            vecx = fvec2;
            SH_LOG(LogTestUtil, Display, TEXT("assignment between FVector and Vector: (%s)."), *vecx.ToString());
            
           // vecx + fvec;
            vecx.yz -= vec.xx;// {1,1,2}
            SH_LOG(LogTestUtil, Display, TEXT("assignment between FVector and Vector: (%s)."), *vecx.ToString());
            
            vecx.xz /= 2;
            SH_LOG(LogTestUtil, Display, TEXT("assignment between FVector and Vector: (%s)."), *vecx.ToString());
            
			//Test compatibility with ue math interfaces.
			{
				FTranslationMatrix tranM(vec);
				SH_LOG(LogTestUtil, Display, TEXT("Test compatibility : (%s)."), *tranM.ToString());
				vec = FMath::VRand();
				SH_LOG(LogTestUtil, Display, TEXT("Test compatibility : (%s)."), *vec.ToString());
			}
		}
        
        //Test assign
        {
            Vector vec1 = { 1,2,3 };
            Vector2f vec2 = { 0.5f,1.1f };
            
            vec1 = vec2.YYX; //{1.1, 1.1, 0.5}
            SH_LOG(LogTestUtil, Display, TEXT("=: (%s)."), *vec1.ToString());
            
            Vector3f vec3 = {0.2f, 0.3f, 0.4f};
            vec1.XY = vec3.YZ; //{0.3,0.4,0.5}
            SH_LOG(LogTestUtil, Display, TEXT("=: (%s)."), *vec1.ToString());
            
            Vector vec4 = vec3.XXX;
            vec1.xy = vec4.xy; //{0.2,0.2,0.5}
            SH_LOG(LogTestUtil, Display, TEXT("=: (%s)."), *vec1.ToString());
        }

		//Test arithmetic operator
		{
			Vector vec1 = { 1,2,3 };
			Vector vec2 = { 1,1.1,1 };
			{
				Vector vec = vec1 + vec2;
				vec += 2;
				SH_LOG(LogTestUtil, Display, TEXT("+: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
			}

			{
				Vector vec = vec1 * vec2;
				vec = 2.0f * vec;
                vec *= 2; // {4, 8.8, 12}
				SH_LOG(LogTestUtil, Display, TEXT("*: (%lf,%lf,%lf)."), vec.X, vec.Y, vec.Z);
			}
            
            {
                Vector vec = 2.0f - vec1;
                SH_LOG(LogTestUtil, Display, TEXT("-: (%s)."), *vec.ToString());
            }
            
			{
				Vector vec = { 1,1,1 };
				(vec += 1) = { 3,3,3 };
				SH_LOG(LogTestUtil, Display, TEXT("+=: (%s)."), *vec.ToString());
			}

			{
				Vector tmp{ 1.233695,0,0 };
				Vector vec{ 1.233698,0,0 };
				SH_LOG(LogTestUtil, Display, TEXT("==: (%d,%d)."), tmp.Equals(vec), tmp.Equals(vec, 1e-5));
			}
		}

		//Test Swizzle
		{
			Vector vec1 = { 1,2,3 };
			Vector vec2 = vec1.XXY; //{1,1,2}
			SH_LOG(LogTestUtil, Display, TEXT("Swizzle: (%lf,%lf,%lf)."), vec2.X, vec2.Y, vec2.Z);
			{
				Vector2D vec3 = vec2.XZ + 3.0; // {4,5}
				SH_LOG(LogTestUtil, Display, TEXT("Swizzle +: (%lf,%lf)."), vec3.X, vec3.Y);
			}
            
            {
                Vector3f vec3 = vec1.zzz - 3.0;
                SH_LOG(LogTestUtil, Display, TEXT("Swizzle -: (%s)."), *vec3.ToString());
            }

			{
				Vector4 vec4 = 2.0 * vec2.ZZZZ; // 2 * {2,2,2,2}
				SH_LOG(LogTestUtil, Display, TEXT("Swizzle *: (%lf,%lf,%lf,%lf)."), vec4.X, vec4.Y, vec4.Z, vec4.W);
			}

			{
				Vector vec4 = vec1 * vec2.ZZZ; //{1,2,3} * {2,2,2}
				SH_LOG(LogTestUtil, Display, TEXT("Swizzle *: (%lf,%lf,%lf)."), vec4.X, vec4.Y, vec4.Z);
			}

			{
				Vector2D vec4 = vec1.ZZ / vec2.XZ; // {3,3} / {1,2}
				SH_LOG(LogTestUtil, Display, TEXT("Swizzle /: (%lf,%lf)."), vec4.X, vec4.Y);
			}
		}

		//Test OutPtr
		{
			struct Unit {
				Unit(int v) : Value(v) {}
				~Unit() { SH_LOG(LogTestUtil, Display, TEXT("~Unit:%d"), Value); }
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
			TestSeq(AUX::MakeRangeIntegerSequence<int, -2, 2>{});
			//Avoid the following code:
			// void SomeFunction()
			// {
			//	int VarFromFile = 5;
			// 	if (VarFromFile == 2) {
			//		UnitTmp<2>() ...
			//	}
			//	else if (VarFromFile == 3) {
			//		UnitTmp<3>() ...
			//	}
			//	.
			//	.
			//	.
			//	else (VarFromFile == 64) {
			//		UnitTmp<64>() ...
			//	}
			// }

			int VarFromFile = 2;
			FString str = "114514";
			RUNTIME_INTEGER_CONV_OneVar(VarFromFile, 2, 4)
			{
				UnitTmp<VarFromFile>{}.Run(str);
			};

			double Result = RUNTIME_INTEGER_CONV_OneVar(VarFromFile, 2, 64) {
				return UnitTmp<VarFromFile>{}.Result;
			};
			SH_LOG(LogTestUtil, Warning, TEXT("Test RUNTIME_INTEGER_CONV: %lf"), Result);

			//Or
			AUX::ConstexprFor<2, 64, 1>([&](auto Index) {
				if(VarFromFile == Index)
				{
					Result = UnitTmp<Index>{}.Result + 1;
					ConstexprForBreak();
				}
				//Anyway, ConstexprForEnd must be added the scope end.
				ConstexprForEnd();
			});
			SH_LOG(LogTestUtil, Warning, TEXT("Test ConstexprFor: %lf"), Result);

			//enum?
			EnumClassTest e = EnumClassTest::B;
			int ee = static_cast<int>(e);
			RUNTIME_INTEGER_CONV_OneVar(ee, 0, static_cast<int>(EnumClassTest::Num))
			{
				UnitTmp2<EnumClassTest(ee())>{}.Run();
			};

			int Var2FromFile = 5;
			//Multiple run-time var
			RUNTIME_INTEGER_CONV_TwoVar(
				VarFromFile, 2, 4,
				Var2FromFile, 5, 7)
			{
				UnitTmp<VarFromFile, Var2FromFile>{};
			};

			//Or
			//ps: error on vs2019
			/*AUX::ConstexprFor<2, 4, 1>([&](auto Index) {
				if (VarFromFile == Index)
				{
					AUX::ConstexprFor<5, 7, 1>([&](auto Index2) {
						if (Var2FromFile == Index2)
						{
							UnitTmp<Index, Index2>{};
						}
						ConstexprForEnd();
					});
				}
				ConstexprForEnd();
			});*/

		}

		{
			Vector&& t = Vector{ 1,2,3 };
			FString s = AUX::GetRawTypeName(&t);
			SH_LOG(LogTestUtil, Display, TEXT("TestTypeName: %s"), *s);

			constexpr auto Pred = [](int Val) {
				for (int i = 2; i * i <= Val; ++i)
				{
					if (Val % i == 0) return false;
				}
				return true;
			};
			TestSeq(AUX::MakeIntegerSequenceByPredicate<int, 10, 30, Pred>());
			TestSeq(AUX::ReverseInegerSequence(TIntegerSequence<int, 2, 3, 5>{}));
		}

		//Test FunctorToFuncPtr
		{
			int t = 233;
			auto f = [t](int a, int b) {
				SH_LOG(LogTestUtil, Display, TEXT("TestFunctorToFuncPtr:%d"), t + a + b);
			};

			TestFunctorToFuncPtr(AUX::FunctorToFuncPtr(f));
		}
        
        //Test STEAL_PRIVATE_MEMBER
        {
            PrivateUnitTest t;
            int& Var = GetPrivate_PrivateUnitTest_Var(t);
            Var = 1;
            SH_LOG(LogTestUtil, Display, TEXT("Test STEAL_PRIVATE_MEMBER:%d"), Var);
        }
		
		//Test CALL_PRIVATE_FUNCTION
		{
			CallPrivate_PrivateUnitTest_test(PrivateUnitTest{}, 123);
            CallPrivate_PrivateUnitTest_overload1(PrivateUnitTest{}, 114514, 1);
            CallPrivate_PrivateUnitTest_overload2(PrivateUnitTest{}, 233.233f);
			CallPrivate_PrivateUnitTestBase_Run(PrivateUnitTest{});
		}

		//Test AUX::initializer_list
		{
			static FString LogInfo;
			struct Unit
			{
				Unit(int) {}
				Unit(const Unit&) {
					LogInfo += " copy";
				}
				Unit(Unit&&) {
					LogInfo += " move";
				}
			};

			auto test = [](AUX::initializer_list<Unit> InitList)
			{
				TArray<Unit> Arr;
				for (const auto& ItemWrapper : InitList)
				{
					Arr.Add(ItemWrapper.Extract());
				}
			};

			SH_LOG(LogTestUtil, Display, TEXT("Test AUX::initializer_list: "));
			{
				Unit t{ 1 };
				test({ t });
				SH_LOG(LogTestUtil, Display, TEXT("Lvalue(%s )"), *LogInfo);
				LogInfo.Empty();
				test({ std::move(t) });
				SH_LOG(LogTestUtil, Display, TEXT("Rvalue(%s )"), *LogInfo);
			}
		}
	}

}

