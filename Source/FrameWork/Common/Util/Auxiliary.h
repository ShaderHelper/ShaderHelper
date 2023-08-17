#pragma once
#include <Templates/RefCounting.h>
#include <Misc/GeneratedTypeName.h>
#include <array>
#include <algorithm>
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
        
        template<typename LType, typename RType>
        static void Assign(LType& lhs, RType& rhs) { ((lhs[LeftIndexes] = rhs[RightIndexes]), ...); }
        template<typename LType, typename RType>
        static void AddAssign(LType& lhs, RType& rhs) { ((lhs[LeftIndexes] += rhs[RightIndexes]), ...); }
        template<typename LType, typename RType>
        static void SubAssign(LType& lhs, RType& rhs) { ((lhs[LeftIndexes] -= rhs[RightIndexes]), ...); }
        template<typename LType, typename RType>
        static void MulAssign(LType& lhs, RType& rhs) { ((lhs[LeftIndexes] *= rhs[RightIndexes]), ...); }
        template<typename LType, typename RType>
        static void DivAssign(LType& lhs, RType& rhs) { ((lhs[LeftIndexes] /= rhs[RightIndexes]), ...); }

    };

    template<typename Type1, typename Type2, typename = void>
    struct GetPromotedArithmeticType;

    template<typename Type1, typename Type2>
    struct GetPromotedArithmeticType<Type1, Type2,
        std::enable_if_t<
            std::is_arithmetic_v<Type1> && std::is_arithmetic_v<Type2>
        >
    >
    {
        using Type = decltype(std::declval<Type1>() + std::declval<Type2>());
    };

    template<typename Type1, typename Type2>
    using GetPromotedArithmeticType_T = typename GetPromotedArithmeticType<Type1, Type2>::Type;

    template<int... Seq>
    constexpr bool HasDuplicateCompImpl()
    {
        constexpr int MaxValue = []()
        {
            std::array<int, sizeof...(Seq)> arr{Seq...};
            return *std::max_element(arr.begin(), arr.end());
        }();
        
        int num[MaxValue + 1]{};
        std::array<int, sizeof...(Seq)> arr{Seq...};
        for(int i = 0; i < arr.size(); i++)
        {
            if(++num[arr[i]] > 1)
            {
                return true;
            }
        }
        return false;
    }

    template<int... Seq>
    struct HasDuplicateComp
    {
        static_assert(sizeof...(Seq) > 0);
        static constexpr bool Value = HasDuplicateCompImpl<Seq...>();
    };

    template<int... Seq>
    inline constexpr bool HasDuplicateComp_V = HasDuplicateComp<Seq...>::Value;

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

    template<int Min, int Max, bool(*Pred)(int), int Size, int... Seq>
    auto MakeIntegerSequeceByPredicateImpl(TIntegerSequence<int, Seq...>) {
        constexpr auto arr = [] {
            std::array<int, Size> arr{};
            int index = 0;
            for (int i = Min; i <= Max; ++i) {
                if (Pred(i)) arr[index++] = i;
            }
            return arr;
        }();
        return TIntegerSequence<int, arr[Seq]...>{};
    }

    //Predicate must be constexpr.
    template<int Min, int Max, bool(*Pred)(int)>
    auto MakeIntegerSequeceByPredicate() {
        constexpr int Size = [] {
            int size = 0;
            for (int i = Min; i <= Max; ++i) {
                if (Pred(i)) size++;
            }
            return size;
        }();
        return MakeIntegerSequeceByPredicateImpl<Min, Max, Pred, Size>(TMakeIntegerSequence<int, Size>{});
    }

    template<typename Func, int... Seq>
   void RunCaseWithInt(int Var, const Func& func, TIntegerSequence<int, Seq...>) {
       (func(Var,std::integral_constant<int, Seq>{}), ...);
    }
		
#define RUNCASE_WITHINT_IMPL(LambName,VarToken,Min,Max,...)                                                     \
    checkf(VarToken >= Min && VarToken <= Max, TEXT("%s must be [%d,%d]"), *FString(#VarToken), Min, Max);      \
    auto LambName = [&](int Var, auto&& t) {                                                                    \
        using BaseType = std::remove_reference_t<decltype(t)>;                                                  \
        constexpr int VarToken = BaseType::value;                                                               \
        if (VarToken == Var) {                                                                                  \
            __VA_ARGS__                                                                                         \
        }                                                                                                       \
    };                                                                                                          \
    AUX::RunCaseWithInt(VarToken, LambName, AUX::MakeRangeIntegerSequence<Min, Max>{});

   //Pass a run-time integer with known small range to template
   //* The large range will lead to code explosion, please carefully choose the range.
   //* `VarToken` must be a simple variable name rather than a complex expression. 
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

    //Returns a FString that stores the type name from pointer or object. (As an alternative to rtti, but just represents the static type name of the expression )
    //Its result is not the same on different platforms, so it should not be saved but just used as a key value.
    template<typename T>
    FString TypeId(const T& Var) {
        using BaseType = std::remove_cv_t<std::remove_pointer_t<T>>;
        return TTypename<BaseType>::Value;
    }
    
    //https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2593r0.html
    template<typename T>
    inline constexpr bool AlwaysFalse = false;

    template<typename T>
    T* GetAddrExt(T&& Val) {
        return std::addressof(Val);
    }

    //For strict-aliasing rule
    template<typename To, typename From>
    To BitCast(const From& src) {
        static_assert(sizeof(To) == sizeof(From), "sizeof(To) != sizeof(From)");
        static_assert(std::is_trivially_copyable_v<To>, "Destination type is not trivially copyable");
        static_assert(std::is_trivially_copyable_v<From>, "Source type is not trivially copyable");
        
        To dst;
        std::memcpy(&dst, &src, sizeof(To));
        return dst;
    }

    template<ESPMode Mode = ESPMode::ThreadSafe, typename T, typename D>
    TSharedPtr<T, Mode> TransOwnerShip(TUniquePtr<T, D>&& InUniquePtr) {
        return TSharedPtr<T, Mode>(InUniquePtr.Release());
    }

    template<typename T, typename U>
    TRefCountPtr<T> StaticCastRefCountPtr(const TRefCountPtr<U>& InRefCountPtr) {
        T* RawPtr = static_cast<T*>(InRefCountPtr.GetReference());
        return TRefCountPtr<T>{RawPtr};
    }
    
	template<typename T>
	struct TraitFuncTypeFromFuncPtr;

	template<typename T, typename C>
	struct TraitFuncTypeFromFuncPtr<T C::*> { using Type = T; };

	template<typename T>
	using TraitFuncTypeFromFunctor_T = typename TraitFuncTypeFromFuncPtr<decltype(&T::operator())>::Type;

	template<typename T1, typename T2>
	struct FunctorExt;

#define DefineFunctorExtWithQualifier(...)                                       \
	template<typename T, typename Ret, typename... ParamType>                    \
	struct FunctorExt<T, Ret(ParamType...) __VA_ARGS__>                          \
	{                                                                            \
		static Ret Call(ParamType... Params)                                     \
		{                                                                        \
			return (*FunctorStorage)(Params...);                                 \
		}                                                                        \
		static T* FunctorStorage;                                                \
	};                                                                           \
	template<typename T, typename Ret, typename... ParamType>                    \
	T* FunctorExt<T, Ret(ParamType...) __VA_ARGS__>::FunctorStorage = nullptr;   

	DefineFunctorExtWithQualifier();
	DefineFunctorExtWithQualifier(const);

	//Convert functor with state to function pointer.
	template<typename T>
	auto FunctorToFuncPtr(T&& Functor)
	{
		using CleanType = std::decay_t<T>;
		using CurFunctorExt = FunctorExt<CleanType, TraitFuncTypeFromFunctor_T<CleanType>>;
		CurFunctorExt::FunctorStorage = &Functor;
		return &CurFunctorExt::Call;
	}

//Access a private member without undefined behavior. (see https://quuxplusone.github.io/blog/2020/12/03/steal-a-private-member/)
#define STEAL_PRIVATE_MEMBER(ClassName, MemberType, MemeberName)                                                    \
    namespace                                                                                                       \
    {                                                                                                               \
        MemberType& GetPrivate_##ClassName##_##MemeberName(ClassName& Object);                                      \
        template <MemberType ClassName::* Ptr> struct Hack_##ClassName##_##MemeberName {                            \
            friend MemberType& GetPrivate_##ClassName##_##MemeberName(ClassName& Object) { return Object.*Ptr; }    \
        };                                                                                                          \
        template struct Hack_##ClassName##_##MemeberName<&ClassName::MemeberName>;                                  \
    };

//Call a private member function without undefined behavior.
//Note: The virtual dispatch will happen if the function is virtual.
#define CALL_PRIVATE_FUNCTION(CalleeName, ClassName, FunctionName, Qualifiers, RetType, ...)                            \
    namespace                                                                                                           \
    {                                                                                                                   \
        using MemFuncPtr_##CalleeName = RetType(ClassName::*)(__VA_ARGS__) Qualifiers;                                  \
        MemFuncPtr_##CalleeName GetPrivate_##CalleeName();                                                              \
        template<MemFuncPtr_##CalleeName Ptr>                                                                           \
        struct Hack_##CalleeName {                                                                                      \
            friend MemFuncPtr_##CalleeName GetPrivate_##CalleeName() { return Ptr; }                                    \
        };                                                                                                              \
        template struct Hack_##CalleeName<static_cast<MemFuncPtr_##CalleeName>(&ClassName::FunctionName)>;              \
                                                                                                                        \
        template<typename T, typename... Args>                                                                          \
        RetType CallPrivate_##CalleeName(T&& Object, Args&&... args)                                                    \
        {                                                                                                               \
            MemFuncPtr_##CalleeName Ptr = GetPrivate_##CalleeName();                                                    \
            return (std::forward<T>(Object).*Ptr)(std::forward<Args>(args)...);                                         \
        }                                                                                                               \
    };

} // end AUX namespace
} // end FRAMEWORK namespace
