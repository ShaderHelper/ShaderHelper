// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Library_hpp
#define imp_Library_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<id<MTLFunction>, void> : public IMPTableBase<id<MTLFunction>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLFunction>>(C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(FunctionType, C)
	, INTERPOSE_CONSTRUCTOR(PatchType, C)
	, INTERPOSE_CONSTRUCTOR(PatchControlPointCount, C)
	, INTERPOSE_CONSTRUCTOR(VertexAttributes, C)
	, INTERPOSE_CONSTRUCTOR(StageInputAttributes, C)
	, INTERPOSE_CONSTRUCTOR(Name, C)
	, INTERPOSE_CONSTRUCTOR(FunctionConstantsDictionary, C)
	, INTERPOSE_CONSTRUCTOR(NewArgumentEncoderWithBufferIndex, C)
	, INTERPOSE_CONSTRUCTOR(NewArgumentEncoderWithBufferIndexreflection, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLFunction>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLFunction>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLFunction>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLFunction>, functionType, FunctionType, MTLFunctionType);
	INTERPOSE_SELECTOR(id<MTLFunction>, patchType, PatchType, MTLPatchType);
	INTERPOSE_SELECTOR(id<MTLFunction>, patchControlPointCount, PatchControlPointCount, NSInteger);
	INTERPOSE_SELECTOR(id<MTLFunction>, vertexAttributes, VertexAttributes, NSArray <MTLVertexAttribute *>*);
	INTERPOSE_SELECTOR(id<MTLFunction>, stageInputAttributes, StageInputAttributes, NSArray <MTLAttribute *>*);
	INTERPOSE_SELECTOR(id<MTLFunction>, name, Name, NSString*);
	INTERPOSE_SELECTOR(id<MTLFunction>, functionConstantsDictionary, FunctionConstantsDictionary, NSDictionary<NSString *, MTLFunctionConstant *> *);
	INTERPOSE_SELECTOR(id<MTLFunction>, newArgumentEncoderWithBufferIndex:, NewArgumentEncoderWithBufferIndex, id<MTLArgumentEncoder>, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLFunction>, newArgumentEncoderWithBufferIndex:reflection:, NewArgumentEncoderWithBufferIndexreflection, id<MTLArgumentEncoder>, NSUInteger, MTLAutoreleasedArgument *);
	
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLFunction>, InterposeClass> : public IMPTable<id<MTLFunction>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLFunction>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLFunction>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(NewArgumentEncoderWithBufferIndex, C);
		INTERPOSE_REGISTRATION(NewArgumentEncoderWithBufferIndexreflection, C);
	}
};

template<>
struct MTLPP_EXPORT IMPTable<id<MTLLibrary>, void> : public IMPTableBase<id<MTLLibrary>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLLibrary>>(C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(NewFunctionWithName, C)
	, INTERPOSE_CONSTRUCTOR(NewFunctionWithNameconstantValueserror, C)
	, INTERPOSE_CONSTRUCTOR(NewFunctionWithNameconstantValuescompletionHandler, C)
	, INTERPOSE_CONSTRUCTOR(FunctionNames, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLLibrary>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLLibrary>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLLibrary>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLLibrary>, newFunctionWithName:, NewFunctionWithName, id <MTLFunction>, NSString*);
	INTERPOSE_SELECTOR(id<MTLLibrary>, newFunctionWithName:constantValues:error:, NewFunctionWithNameconstantValueserror, id <MTLFunction>, NSString*, MTLFunctionConstantValues*, NSError **);
	INTERPOSE_SELECTOR(id<MTLLibrary>, newFunctionWithName:constantValues:completionHandler:, NewFunctionWithNameconstantValuescompletionHandler, void, NSString*, MTLFunctionConstantValues*, void (^)(id<MTLFunction> __nullable function, NSError* error));
	INTERPOSE_SELECTOR(id<MTLLibrary>, functionNames, FunctionNames, NSArray <NSString *> *);
};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLLibrary>, InterposeClass> : public IMPTable<id<MTLLibrary>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLLibrary>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLLibrary>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(NewFunctionWithName, C);
		INTERPOSE_REGISTRATION(NewFunctionWithNameconstantValueserror, C);
		INTERPOSE_REGISTRATION(NewFunctionWithNameconstantValuescompletionHandler, C);
	}
};

MTLPP_END

#endif /* imp_Library_hpp */
