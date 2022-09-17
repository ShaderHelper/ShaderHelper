// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

namespace Algo
{
	/**
	 * Rearranges the elements so that all the elements for which Predicate returns true precede all those for which it returns false.  (not stable)
	 *
	 * @param	First		pointer to the first element
	 * @param	Num			the number of items
	 * @param	Predicate	unary predicate class
	 * @return	index of the first element in the second group
	 */
	template<class T, typename IndexType, class UnaryPredicate>
	IndexType Partition(T* Elements, const IndexType Num, const UnaryPredicate& Predicate)
	{
		T* First = Elements;
		T* Last = Elements + Num;
		
		while (First != Last) 
		{
			while (Predicate(*First)) 
			{
				++First;
				if (First == Last) 
				{	
					return (IndexType)(First - Elements);
				}
			}
		
			do 
			{
				--Last;
				if (First == Last)
				{
					return (IndexType)(First - Elements);
				}
			} while (!Predicate(*Last));
		
			Exchange(*First, *Last);
			++First;
		}
	
		return (IndexType)(First - Elements);
	}
} //namespace Algo