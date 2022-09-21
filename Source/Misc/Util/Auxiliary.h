#pragma once
#include "CommonHeader.h"

namespace SH {
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


	}	
}