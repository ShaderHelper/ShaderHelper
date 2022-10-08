#pragma once
#include "CommonHeader.h"

namespace FRAMEWORK {
	namespace AUX {

		template<typename LeftSeq, typename RightSeq>
		struct Op;

		template<uint32 ... LeftIndexes, uint32 ... RightIndexes>
		struct Op<TIntegerSequence<uint32, LeftIndexes...>, TIntegerSequence<uint32, RightIndexes...>> {

			static_assert(sizeof...(LeftIndexes) == sizeof...(RightIndexes), "Op just support same size sequence.");
			
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

		template<typename Smart, typename Pointer,
			std::enable_if_t<std::is_same_v<Pointer, void>, bool> = true
		>
		decltype(auto) BaseOutPtr(Smart& s) {
			return OutPtrImpl<Smart, Smart::ElementType*>(s);
		}

		template<typename Smart, typename Pointer,
			std::enable_if_t<!std::is_same_v<Pointer, void>, bool> = true
		>
		decltype(auto) BaseOutPtr(Smart& s) {
			return OutPtrImpl<Smart, Pointer>(s);
		}

		//c++23 OutPtr for UE smart pointer.
		template<typename Pointer = void, typename Smart>
		decltype(auto) OutPtr(Smart& s) {
			return BaseOutPtr<Smart,Pointer>(s);
		}

	}	
}