// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class UClass;
class UScriptStruct;
class UEnum;

/*-----------------------------------------------------------------------------
C++ templated Static(Class/Struct/Enum) retrieval function prototypes.
-----------------------------------------------------------------------------*/

template<typename ClassType>	UClass*			StaticClass();
template<typename StructType>	UScriptStruct*	StaticStruct();
template<typename EnumType>		UEnum*			StaticEnum();
