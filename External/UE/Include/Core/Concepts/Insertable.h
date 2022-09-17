// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Describes an insertion operation for a destination type where an instance of another type can be inserted via operator<<.
 */
template <typename DestType>
struct CInsertable {
	template <typename T>
	auto Requires(DestType Dest, T& Val) -> decltype(
		Dest << Val
	);
};
