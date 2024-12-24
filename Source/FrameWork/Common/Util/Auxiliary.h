#pragma once
#include <Templates/RefCounting.h>
#include <Misc/GeneratedTypeName.h>
#include <array>
#include <algorithm>
#include <cstring>
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

	//std::unreachable
	[[noreturn]] FORCEINLINE void Unreachable()
	{
		check(false);
#if PLATFORM_WINDOWS
		__assume(false);
#else
		__builtin_unreachable();
#endif
	}

    template<typename T, T Min, T... Seq>
    struct RangeIntegerSequenceImpl {
        using Type = TIntegerSequence<T, (Seq + Min)...>;
    };

    template<typename T, T Min, typename Seq>
    struct RangeIntegerSequence;

    template<typename T, T Min, T... Seq>
    struct RangeIntegerSequence<T, Min, TIntegerSequence<T, Seq...>> {
        static_assert(sizeof...(Seq) > 0, "Min must be less than Max");
        using Type = typename RangeIntegerSequenceImpl<T, Min, Seq...>::Type;
    };
    
    //Make TIntegerSequence[min,max]
    template<typename T, T Min, T Max>
    using MakeRangeIntegerSequence = typename RangeIntegerSequence<T, Min, TMakeIntegerSequence<T, Max - Min + 1>>::Type;

	template<typename T, T... Seq, int... Indexes>
	auto ReverseInegerSequenceImpl(TIntegerSequence<T, Seq...> IntegerSeq, TIntegerSequence<int, Indexes...> IndexesSeq)
	{
		constexpr auto arr = [] {
			std::array<T, sizeof...(Seq)> target{};
			std::array<T, sizeof...(Seq)> source{ Seq... };
			for (int index = 0; index < target.size(); index++)
			{
				target[index] = source[target.size() - index - 1];
			}
			return target;
		}();
		return TIntegerSequence<T, arr[Indexes]...>{};
	}

	template<typename T, T... Seq>
	auto ReverseInegerSequence(TIntegerSequence<T, Seq...> IntegerSeq)
	{
		return ReverseInegerSequenceImpl(IntegerSeq, TMakeIntegerSequence<int, sizeof...(Seq)>{});
	}

    template<typename T, T Min, T Max, bool(*Pred)(T), int Size, int... Seq>
    auto MakeIntegerSequenceByPredicateImpl(TIntegerSequence<T, Seq...>) {
        constexpr auto arr = [] {
            std::array<T, Size> arr{};
            int index = 0;
            for (T i = Min; i <= Max; ++i) {
                if (Pred(i)) arr[index++] = i;
            }
            return arr;
        }();
        return TIntegerSequence<T, arr[Seq]...>{};
    }

    //Predicate must be constexpr.
    template<typename T, T Min, T Max, bool(*Pred)(T)>
    auto MakeIntegerSequenceByPredicate() {
        constexpr int Size = [] {
            int size = 0;
            for (T i = Min; i <= Max; ++i) {
                if (Pred(i)) size++;
            }
            return size;
        }();
        return MakeIntegerSequenceByPredicateImpl<T, Min, Max, Pred, Size>(TMakeIntegerSequence<int, Size>{});
    }

	//Returns a FString that stores the type name. (As an alternative to rtti, but just represents the static type name)
	//Its result may not be same on different platforms, so it should not be saved but just used as a key value.
    template<typename T>
    struct TTypename {
        static inline FString Value = GetGeneratedTypeName<T>();
    };

    template<typename T>
    FString GetRawTypeName(const T& Var = nullptr) {
        using RawType = std::remove_cv_t<std::remove_pointer_t<T>>;
        return TTypename<RawType>::Value;
    }

	template<typename T>
	inline const FString TypeName = TTypename<T>::Value;

	template<typename T>
	inline const FString RawTypeName = GetRawTypeName<T>();
    
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

	template<typename T, typename U>
	TUniquePtr<T> StaticCastUniquePtr(TUniquePtr<U>&& InUniquePtr) {
		return TUniquePtr<T>{ InUniquePtr.Release() };
	}
    
	template<typename T>
	struct TraitFuncTypeFromFuncPtr;
	template<typename FuncType, typename C>
	struct TraitFuncTypeFromFuncPtr<FuncType C::*> { using Type = FuncType; };

	template<typename Ret, typename... ParamType>
	struct TraitRetAndParamTypeFromFuncTypeBase { using Type = std::tuple<Ret, ParamType...>; };
	template<typename FuncType>
	struct TraitRetAndParamTypeFromFuncType;
	template<typename Ret, typename... ParamType>
	struct TraitRetAndParamTypeFromFuncType<Ret(ParamType...)> : TraitRetAndParamTypeFromFuncTypeBase<Ret, ParamType...> {};
	template<typename Ret, typename... ParamType>
	struct TraitRetAndParamTypeFromFuncType<Ret(ParamType...) const> : TraitRetAndParamTypeFromFuncTypeBase<Ret, ParamType...> {};
	template<typename FuncType>
	using TraitRetAndParamTypeFromFuncType_T = typename TraitRetAndParamTypeFromFuncType<FuncType>::Type;

	template<typename RetAndParamTypeList>
	struct TraitRetFromRetAndParamTypeList;
	template<typename Ret, typename... ParamType>
	struct TraitRetFromRetAndParamTypeList<std::tuple<Ret, ParamType...>> { using Type = Ret; };
	template<typename RetAndParamTypeList>
	using TraitRetFromRetAndParamTypeList_T = typename TraitRetFromRetAndParamTypeList<RetAndParamTypeList>::Type;

	template<typename Func>
	using TraitFuncTypeFromFunctor_T = typename TraitFuncTypeFromFuncPtr<decltype(&Func::operator())>::Type;

	template<typename Func, typename RetAndParamTypeList>
	struct FunctorExt;
	template<typename Func, typename Ret, typename... ParamType>
	struct FunctorExt<Func, std::tuple<Ret, ParamType...>>
	{                                                                            
		static Ret Call(ParamType... Params)                                     
		{                                                                        
			return (*FunctorStorage)(Params...);                                 
		}                                                                        
		static inline Func* FunctorStorage = nullptr;
	};                                                                           

	//Convert the functor(non-generic lambda) with state to a function pointer.
	template<typename T>
	auto FunctorToFuncPtr(T&& Functor)
	{
		using CleanType = std::decay_t<T>;
		using RetAndParamTypeList = TraitRetAndParamTypeFromFuncType_T<TraitFuncTypeFromFunctor_T<CleanType>>;
		using CurFunctorExt = FunctorExt<CleanType, RetAndParamTypeList>;
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

	template<typename T>
	class InitListWrapper : FNoncopyable
	{
	public:
		InitListWrapper(const T& InValue) : ValueRef(&InValue) {}
		InitListWrapper(T&& InValue)
		{
			new (&Value) T(std::move(InValue));
		}

		template<typename... Args, typename Arg0 = std::tuple_element_t<0, std::tuple<std::decay_t<Args>...>>,
			typename = std::enable_if_t<!std::is_same_v<T, Arg0>>,
			typename = std::enable_if_t<std::is_constructible_v<T, Args...>>
		>
		InitListWrapper(Args&&... InArg)
		{
			new (&Value) T(std::forward<Args>(InArg)...);
		}

		T Extract() const
		{
			if constexpr (std::is_copy_constructible_v<T>)
			{
				if (ValueRef) {
					return *ValueRef;
				}
			}
			return reinterpret_cast<T&&>(Value);
		}

	private:
		mutable TTypeCompatibleBytes<T> Value;
		const T* ValueRef = nullptr;
	};

	//Set up AUX::initializer_list if you want to move elements from initializer_list.
	template<typename T>
	using initializer_list = std::initializer_list<InitListWrapper<T>>;

	template<typename T>
	TArray<T> MakeTArray(AUX::initializer_list<T> InitList)
	{
		TArray<T> Arr;
		Arr.Reserve(InitList.size());
		for (const auto& ItemWrapper : InitList)
		{
			Arr.Add(ItemWrapper.Extract());
		}
		return Arr;
	}

	template<typename F, typename RetAndParamTypeList>
	struct RunTimeConvCall;			
	template<typename F, typename Ret>
	struct RunTimeConvCallBase
	{
		virtual Ret Call() = 0;
		virtual ~RunTimeConvCallBase() = default;
	};
	template<typename F, typename Ret, typename... ParamTypes>
	struct RunTimeConvCall<F, std::tuple<Ret, ParamTypes...>> : RunTimeConvCallBase<F, Ret>
	{
		RunTimeConvCall(const F* InLamb)
			:Lamb(InLamb) {}

		template<int... Indexes>
		Ret CallImpl(TIntegerSequence<int, Indexes...>)
		{
			return (*Lamb)(std::get<Indexes>(Params)...);
		}

		Ret Call() override
		{
			ON_SCOPE_EXIT
			{
				Lamb = nullptr;
			};
			return CallImpl(TMakeIntegerSequence<int, sizeof...(ParamTypes)>{});
		}
		~RunTimeConvCall()
		{
			if (Lamb)
			{
				CallImpl(TMakeIntegerSequence<int, sizeof...(ParamTypes)>{});
			}
		}
		const F* Lamb;
		std::tuple<ParamTypes...> Params;
	};

	template<typename Func, typename Ret>
	struct RunTimeConvCallRet
	{
		operator Ret() const
		{
			return CallPtr->Call();
		}

		~RunTimeConvCallRet()
		{
			delete CallPtr;
		}

		RunTimeConvCallBase<Func, Ret>* CallPtr;
	};

	template<typename VarType, VarType Min, VarType Max>
	struct RunTimeConvWrapper
	{
		RunTimeConvWrapper(const FString& VarName, VarType InVar) : Var(InVar) 
		{
			checkf(InVar >= Min && InVar <= Max, TEXT("%s:%d must be [%d,%d]"), *VarName, InVar, Min, Max);
		}
	
		VarType Var;
	};

	template<
		int CurVarWrapperIndex,
		typename... ResultTypes,
		typename RunTimeConvWrapperTuple,
		typename VarType, 
		VarType Cur, VarType Min, VarType Max, 
		typename Func
	>
	auto RunTimeConvImpl(
		const RunTimeConvWrapperTuple& RunTimeConvWrappers,
		const RunTimeConvWrapper<VarType, Min, Max>& CurVarWrapper, 
		const Func& Lamb,
		std::integral_constant<VarType, Cur>
		) 
	{
		VarType Var = CurVarWrapper.Var;
		if (Var == Cur)
		{
			if constexpr (CurVarWrapperIndex + 1 < std::tuple_size_v<RunTimeConvWrapperTuple>)
			{
				return RunTimeConvImpl<CurVarWrapperIndex + 1, ResultTypes... , std::integral_constant<VarType, Cur>>
					(RunTimeConvWrappers, std::get<CurVarWrapperIndex + 1>(RunTimeConvWrappers), Lamb);
			}
			else
			{
				//std::integral_constant: Lambda does not support non-type template parameter until c++20.
				using FuncPtr = decltype(&Func::template operator()<ResultTypes..., std::integral_constant<VarType, Cur>>);
				using RetAndParamTypeList = TraitRetAndParamTypeFromFuncType_T<typename TraitFuncTypeFromFuncPtr<FuncPtr>::Type>;
				using Ret = TraitRetFromRetAndParamTypeList_T<RetAndParamTypeList>;
				//Erasure param types to make the same return type.
				RunTimeConvCallBase<Func, Ret>* CallPtr = new RunTimeConvCall<Func, RetAndParamTypeList>{ &Lamb };
				return RunTimeConvCallRet<Func, Ret>{CallPtr};
			}
	
		}
		else if constexpr (Cur < Max)
		{
			return RunTimeConvImpl<CurVarWrapperIndex, ResultTypes...>(RunTimeConvWrappers, std::get<CurVarWrapperIndex>(RunTimeConvWrappers), Lamb, std::integral_constant<VarType, Cur + 1>{});
		}
		else
		{
			Unreachable();
		}
	}

	template<
		int CurVarWrapperIndex,
		typename... ResultTypes,
		typename RunTimeConvWrapperTuple,
		typename VarType,
		VarType Min, VarType Max,
		typename Func,
		VarType Cur = Min
	>
	auto RunTimeConvImpl(
		const RunTimeConvWrapperTuple& RunTimeConvWrappers,
		const RunTimeConvWrapper<VarType, Min, Max>& CurVarWrapper,
		const Func& Lamb
		)
	{
		return RunTimeConvImpl<CurVarWrapperIndex, ResultTypes...>(RunTimeConvWrappers, CurVarWrapper, Lamb, std::integral_constant<VarType, Cur>{});
	}

	template<typename... RunTimeConvWrapperTypes>
	struct MultiRunTimeConvWrapper                    
	{
		template<typename F>
		auto operator+(F&& InLamb)
		{
			return RunTimeConvImpl<0>(RunTimeConvWrappers, std::get<0>(RunTimeConvWrappers), InLamb);
		}
		std::tuple<RunTimeConvWrapperTypes...>RunTimeConvWrappers;
	};

#define RUNTIMECONVWRAPPER_TYPE_1(VarToken,Min,Max) AUX::RunTimeConvWrapper<decltype(VarToken), decltype(VarToken)(Min), decltype(VarToken)(Max)>
#define RUNTIMECONVWRAPPER_TYPE_2(VarToken1,Min1,Max1,VarToken2,Min2,Max2) RUNTIMECONVWRAPPER_TYPE_1(VarToken1, Min1, Max1),RUNTIMECONVWRAPPER_TYPE_1(VarToken2, Min2, Max2)

#define RUNTIMECONVWRAPPER_VALUE_1(VarToken) {#VarToken, VarToken}
#define RUNTIMECONVWRAPPER_VALUE_2(VarToken1, VarToken2) RUNTIMECONVWRAPPER_VALUE_1(VarToken1),RUNTIMECONVWRAPPER_VALUE_1(VarToken2)

#define RUNTIMECONVWRAPPER_LAMB_PARAM_1(VarToken) auto VarToken
#define RUNTIMECONVWRAPPER_LAMB_PARAM_2(VarToken1, VarToken2) RUNTIMECONVWRAPPER_LAMB_PARAM_1(VarToken1),RUNTIMECONVWRAPPER_LAMB_PARAM_1(VarToken2)

#define RUNTIME_INTEGER_CONV_IMPL(RunTimeConvWrapperTypes, RunTimeConvWrapperValues, RunTimeConvWrapperLambParam) \
	AUX::MultiRunTimeConvWrapper<RunTimeConvWrapperTypes> { {RunTimeConvWrapperValues} } + [&](RunTimeConvWrapperLambParam)

	//Magic: Pass run-time integers with known small range to template
	//* The large range maybe lead to code bloat, please carefully choose the range.
	//* `VarToken` must be a simple variable name rather than a complex expression. 
	//	Example: 
	//	int Var = xxx; RUNTIME_INTEGER_CONV_OneVar(Var,...) (√) 
	//	struct A{static int Var;} RUNTIME_INTEGER_CONV_OneVar(A::Var,...) (×)
	//	struct A{static int Var;} int Var = A::Var; RUNTIME_INTEGER_CONV_OneVar(Var,...) (√) 
#define RUNTIME_INTEGER_CONV_OneVar(VarToken,Min,Max) RUNTIME_INTEGER_CONV_IMPL(RUNTIMECONVWRAPPER_TYPE_1(VarToken,Min,Max),RUNTIMECONVWRAPPER_VALUE_1(VarToken),RUNTIMECONVWRAPPER_LAMB_PARAM_1(VarToken))
#define RUNTIME_INTEGER_CONV_TwoVar(VarToken1,Min1,Max1,VarToken2,Min2,Max2) RUNTIME_INTEGER_CONV_IMPL(RUNTIMECONVWRAPPER_TYPE_2(VarToken1,Min1,Max1,VarToken2,Min2,Max2),RUNTIMECONVWRAPPER_VALUE_2(VarToken1,VarToken2),RUNTIMECONVWRAPPER_LAMB_PARAM_2(VarToken1,VarToken2))


	enum class ConstexprForState
	{
		Normal,
		Break,
		Continue
	};
	template<auto Start, auto End, auto Inc, typename Func>
	void ConstexprFor(const Func& InFunc)
	{
		using ParamType = std::integral_constant<decltype(Start), Start>;
		constexpr bool IsProperFormat = std::is_invocable_r_v<ConstexprForState, Func, ParamType>;
		static_assert(IsProperFormat, "Improper functor format.");
		if constexpr (Start <= End)
		{
			ConstexprForState State = InFunc(ParamType{});
			if (State != ConstexprForState::Break)
			{
				ConstexprFor<Start + 1, End, Inc>(InFunc);
			}
		}
	}

#define ConstexprForEnd() return AUX::ConstexprForState::Normal;
#define ConstexprForBreak() return AUX::ConstexprForState::Break;
#define ConstexprForContinue()	return AUX::ConstexprForState::Continue;


} // end AUX namespace
} // end FRAMEWORK namespace
