#pragma once

#define JOIN_IMPL(A, B) A##B
#define JOIN(A, B) JOIN_IMPL(A, B)
#define EXPAND(...) __VA_ARGS__

template<typename T, typename U>
struct is_same
{
	static const bool value = false;
};
template<typename T>
struct is_same<T, T>
{
	static const bool value = true;
};

