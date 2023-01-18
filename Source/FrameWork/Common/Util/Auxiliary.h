#pragma once
#include <Misc/GeneratedTypeName.h>
namespace FRAMEWORK
{
namespace AUX
{
		
    //Support Arithmetic operations on two objects that have the subscript operator via the parameter pack provided by TIntegerSequence
    template<typename LeftSeq, typename RightSeq>
    struct Op;

    template<uint32 ... LeftIndexes, uint32 ... RightIndexes>
    struct Op<TIntegerSequence<uint32, LeftIndexes...>, TIntegerSequence<uint32, RightIndexes...>> {

        static_assert(sizeof...(LeftIndexes) == sizeof...(RightIndexes), "Op just supports the same size sequence.");
        
        template<typename RetType, typename LType, typename RType>
        static RetType Sum(LType& lhs, RType& rhs) { return RetType(lhs[LeftIndexes] + rhs[RightIndexes]...); }
        template<typename RetType, typename LType, typename RType>
        static RetType Mul(LType& lhs, RType& rhs) { return RetType(lhs[LeftIndexes] * rhs[RightIndexes]...); }
        template<typename RetType, typename LType, typename RType>
        static RetType Sub(LType& lhs, RType& rhs) { return RetType(lhs[LeftIndexes] - rhs[RightIndexes]...); }
        template<typename RetType, typename LType, typename RType>
        static RetType Div(LType& lhs, RType& rhs) { return RetType(lhs[LeftIndexes] / rhs[RightIndexes]...); }

    };

    template<typename Smart, typename Pointer>
    struct OutPtrImpl;
    
    template<typename T, typename D, typename Pointer>
    struct OutPtrImpl<TUniquePtr<T, D>, Pointer> {
        using Smart = TUniquePtr<T, D>;
        using Storage = typename Smart::ElementType*;
    public:
        OutPtrImpl(Smart& smart) : SmartPtr(smart), TargetPtr(nullptr) {}
        operator Pointer* () {
            return &TargetPtr;
        }
        ~OutPtrImpl() {
            SmartPtr.Reset(static_cast<Storage>(TargetPtr));
        }
    private:
        Smart& SmartPtr;
        Pointer TargetPtr;
    };

    template<typename T, ESPMode Mode, typename Pointer>
    struct OutPtrImpl<TSharedPtr<T, Mode>, Pointer> {
        using Smart = TSharedPtr<T, Mode>;
        using Storage = typename Smart::ElementType*;
    public:
        OutPtrImpl(Smart& smart) : SmartPtr(smart), TargetPtr(nullptr) {}
        operator Pointer* () {
            return &TargetPtr;
        }
        ~OutPtrImpl() {
            Smart Temp = Smart(static_cast<Storage>(TargetPtr));
            Swap(Temp, SmartPtr);
        }
    private:
        Smart& SmartPtr;
        Pointer TargetPtr;
    };

    template<typename T, ESPMode Mode, typename Pointer>
    struct OutPtrImpl<TSharedRef<T, Mode>, Pointer> {
        using Smart = TSharedRef<T, Mode>;
        using Storage = typename Smart::ElementType*;
    public:
        OutPtrImpl(Smart& smart) : SmartPtr(smart), TargetPtr(nullptr) {}
        operator Pointer* () {
            return &TargetPtr;
        }
        ~OutPtrImpl() {
            Smart Temp = Smart(static_cast<Storage>(TargetPtr));
            Swap(Temp, SmartPtr);
        }
    private:
        Smart& SmartPtr;
        Pointer TargetPtr;
    };

    //c++14
    //template<typename Smart, typename Pointer,
    //	std::enable_if_t<std::is_same_v<Pointer, void>, bool> = true
    //>
    //decltype(auto) BaseOutPtr(Smart& s) {
    //	return OutPtrImpl<Smart, typename Smart::ElementType*>(s);
    //}

    //template<typename Smart, typename Pointer,
    //	std::enable_if_t<!std::is_same_v<Pointer, void>, bool> = true
    //>
    //decltype(auto) BaseOutPtr(Smart& s) {
    //	return OutPtrImpl<Smart, Pointer>(s);
    //}

    //c++23 OutPtr for UE smart pointer.
    template<typename Pointer = void, typename Smart>
    decltype(auto) OutPtr(Smart& s) {
        if constexpr (std::is_same_v<Pointer, void>) {
            return OutPtrImpl<Smart, typename Smart::ElementType*>(s);
        }
        else {
            return OutPtrImpl<Smart, Pointer>(s);
        }
    }

    template<int Min, int... Seq>
    struct RangeIntegerSequenceImpl {
		using Type = TIntegerSequence<int, (Seq + Min)...>;
    };

    template<int Min, typename Seq>
    struct RangeIntegerSequence;

    template<int Min, int... Seq>
    struct RangeIntegerSequence<Min, TIntegerSequence<int, Seq...>> {
        static_assert(sizeof...(Seq) > 0, "Min must be less than Max");
        using Type = typename RangeIntegerSequenceImpl<Min, Seq...>::Type;
    };
    
    //Make TIntegerSequence[min,max]
    template<int Min, int Max>
    using MakeRangeIntegerSequence = typename RangeIntegerSequence<Min, TMakeIntegerSequence<int, Max - Min + 1>>::Type;

    template<typename Func, int... Seq>
   void RunCaseWithInt(int Var, const Func& func, TIntegerSequence<int, Seq...>) {
	   (func(Var,std::integral_constant<int, Seq>{}), ...);
    }

#define RUNCASE_WITHINT_IMPL(LambName,VarToken,Min,Max,...)		\
	checkf(VarToken >= Min && VarToken <= Max, TEXT("%s must be [%d,%d]"), *FString(#VarToken), Min, Max);		\
	auto LambName = [&](int Var, auto&& t) {		\
		constexpr int VarToken = t.value;		\
		if (VarToken == Var) {		\
			##__VA_ARGS__		\
		}		\
	};		\
	AUX::RunCaseWithInt(VarToken, LambName, AUX::MakeRangeIntegerSequence<Min, Max>{});

   //Pass a run-time integer with known small range to template
   //* The large range will lead to code explosion, please carefully choose the range.
   //* `VarToken` must be a variable name rather than a complex expression. 
   //	Example: 
   //	int Var = xxx; RUNCASE_WITHINT(Var,...) (√) 
   //	struct A{static int Var;} RUNCASE_WITHINT(A::Var,...) (×)
   //	struct A{static int Var;} int Var = A::Var; RUNCASE_WITHINT(Var,...) (√) 
#define RUNCASE_WITHINT(VarToken,Min,Max,...)	\
	RUNCASE_WITHINT_IMPL(PREPROCESSOR_JOIN(Auto_Instance_,__COUNTER__),VarToken,Min,Max,__VA_ARGS__) 
	

	template<typename T>
	struct TTypename {
		static inline FString Value = GetGeneratedTypeName<T>();
	};

	//Returns a FString that stores the type name from pointer or object. (As an alternative to rtti)
	//Its result is not the same on different platforms, so it should not be saved but just used as a key value.
	template<typename T>
	FString TypeId(const T& Var) {
		using BaseType = std::remove_cv_t<std::remove_pointer_t<T>>;
		return TTypename<BaseType>::Value;
	}


} // end AUX namespace
} // end FRAMEWORK namespace
