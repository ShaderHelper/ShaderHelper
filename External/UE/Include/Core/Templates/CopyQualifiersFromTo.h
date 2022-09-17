// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Copies the cv-qualifiers from one type to another, e.g.:
 *
 * TCopyQualifiersFromTo<const    T1,       T2>::Type == const T2
 * TCopyQualifiersFromTo<volatile T1, const T2>::Type == const volatile T2
 */
template <typename From, typename To> struct TCopyQualifiersFromTo                          { typedef                To Type; };
template <typename From, typename To> struct TCopyQualifiersFromTo<const          From, To> { typedef const          To Type; };
template <typename From, typename To> struct TCopyQualifiersFromTo<      volatile From, To> { typedef       volatile To Type; };
template <typename From, typename To> struct TCopyQualifiersFromTo<const volatile From, To> { typedef const volatile To Type; };
