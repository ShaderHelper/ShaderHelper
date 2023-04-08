// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef declare_h
#define declare_h

#include "defines.hpp"

MTLPP_BEGIN

typedef struct __IOSurface* IOSurfaceRef;
typedef long NSInteger;
typedef unsigned long NSUInteger;
#if __OBJC_BOOL_IS_BOOL
typedef bool BOOL;
#else
typedef signed char BOOL;
#endif

typedef struct objc_class *Class;
typedef struct objc_method *Method;
typedef struct objc_selector *SEL;
#ifdef __OBJC__
@class Protocol;
#else
struct objc_object { Class isa; };
typedef struct objc_object Protocol;
typedef struct objc_object*id;
#endif
#if !OBJC_OLD_DISPATCH_PROTOTYPES
typedef void (*IMP)(void);
#else
typedef id (*IMP)(id, SEL, ...);
#endif

MTLPP_CLASS(CAMetalLayer);
MTLPP_CLASS(MTLArgument);
MTLPP_CLASS(MTLArgumentDescriptor);
MTLPP_CLASS(MTLAttributeDescriptor);
MTLPP_CLASS(MTLBufferLayoutDescriptor);
MTLPP_CLASS(MTLArrayType);
MTLPP_CLASS(MTLAttribute);
MTLPP_CLASS(MTLCaptureManager);
MTLPP_CLASS(MTLCompileOptions);
MTLPP_CLASS(MTLComputePipelineReflection);
MTLPP_CLASS(MTLComputePipelineDescriptor);
MTLPP_CLASS(MTLDepthStencilDescriptor);
MTLPP_CLASS(MTLFunctionConstant);
MTLPP_CLASS(MTLFunctionConstantValues);
MTLPP_CLASS(MTLHeapDescriptor);
MTLPP_CLASS(MTLPipelineBufferDescriptor);
MTLPP_CLASS(MTLPointerType);
MTLPP_CLASS(MTLRenderPassAttachmentDescriptor);
MTLPP_CLASS(MTLRenderPassColorAttachmentDescriptor);
MTLPP_CLASS(MTLRenderPassDepthAttachmentDescriptor);
MTLPP_CLASS(MTLRenderPassDescriptor);
MTLPP_CLASS(MTLRenderPassStencilAttachmentDescriptor);
MTLPP_CLASS(MTLRenderPipelineColorAttachmentDescriptor);
MTLPP_CLASS(MTLRenderPipelineDescriptor);
MTLPP_CLASS(MTLRenderPipelineReflection);
MTLPP_CLASS(MTLSamplerDescriptor);
MTLPP_CLASS(MTLStageInputOutputDescriptor);
MTLPP_CLASS(MTLStencilDescriptor);
MTLPP_CLASS(MTLStructMember);
MTLPP_CLASS(MTLStructType);
MTLPP_CLASS(MTLTextureDescriptor);
MTLPP_CLASS(MTLTextureReferenceType);
MTLPP_CLASS(MTLTileRenderPipelineColorAttachmentDescriptor);
MTLPP_CLASS(MTLTileRenderPipelineDescriptor);
MTLPP_CLASS(MTLType);
MTLPP_CLASS(MTLVertexAttribute);
MTLPP_CLASS(MTLVertexAttributeDescriptor);
MTLPP_CLASS(MTLVertexBufferLayoutDescriptor);
MTLPP_CLASS(MTLVertexDescriptor);
MTLPP_CLASS(MTLBufferLayoutDescriptorArray);
MTLPP_CLASS(MTLAttributeDescriptorArray);
MTLPP_CLASS(MTLRenderPassColorAttachmentDescriptorArray);
MTLPP_CLASS(MTLRenderPipelineColorAttachmentDescriptorArray);
MTLPP_CLASS(MTLPipelineBufferDescriptorArray);
MTLPP_CLASS(MTLVertexBufferLayoutDescriptorArray);
MTLPP_CLASS(MTLVertexAttributeDescriptorArray);

#if __OBJC__
@class NSArray<__covariant ObjectType>;
@class NSDictionary<__covariant KeyType, __covariant ObjectType>;
#else
template<typename T> class NSArray;
template<typename K, typename V> class NSDictionary;
#endif

MTLPP_CLASS(NSBundle);
MTLPP_CLASS(NSCondition);
MTLPP_CLASS(NSError);
MTLPP_CLASS(NSObject);
MTLPP_CLASS(NSString);
MTLPP_CLASS(NSTask);
MTLPP_CLASS(NSURL);

MTLPP_CLASS(MTLPPResourceUsageValidator);

MTLPP_PROTOCOL(OS_dispatch_data);
typedef NSObject<OS_dispatch_data>* dispatch_data_t;

MTLPP_PROTOCOL(CAMetalDrawable);
MTLPP_PROTOCOL(MTLArgumentEncoder);
MTLPP_PROTOCOL(MTLBlitCommandEncoder);
MTLPP_PROTOCOL(MTLBuffer);
MTLPP_PROTOCOL(MTLCaptureScope);
MTLPP_PROTOCOL(MTLCommandQueue);
MTLPP_PROTOCOL(MTLCommandBuffer);
MTLPP_PROTOCOL(MTLCommandEncoder);
MTLPP_PROTOCOL(MTLComputeCommandEncoder);
MTLPP_PROTOCOL(MTLComputePipelineState);
MTLPP_PROTOCOL(MTLDepthStencilState);
MTLPP_PROTOCOL(MTLDevice);
MTLPP_PROTOCOL(MTLDrawable);
MTLPP_PROTOCOL(MTLFence);
MTLPP_PROTOCOL(MTLFunction);
MTLPP_PROTOCOL(MTLHeap);
MTLPP_PROTOCOL(MTLLibrary);
MTLPP_PROTOCOL(MTLParallelRenderCommandEncoder);
MTLPP_PROTOCOL(MTLRenderCommandEncoder);
MTLPP_PROTOCOL(MTLRenderPipelineState);
MTLPP_PROTOCOL(MTLResource);
MTLPP_PROTOCOL(MTLSamplerState);
MTLPP_PROTOCOL(MTLTexture);
MTLPP_PROTOCOL(MTLSamplerState);
MTLPP_PROTOCOL(NSObject);

#if (__cplusplus)
#define MTLPP_OPTIONS(_name, _type) typedef _type _name
#else
#define MTLPP_OPTIONS(_name, _type) typedef enum _name : _type _name
#endif

MTLPP_OPTIONS(MTLBlitOption, NSUInteger);
MTLPP_OPTIONS(MTLRenderStages, NSUInteger);
MTLPP_OPTIONS(MTLResourceOptions, NSUInteger);
MTLPP_OPTIONS(MTLResourceUsage, NSUInteger);
MTLPP_OPTIONS(MTLStoreActionOptions, NSUInteger);
MTLPP_OPTIONS(MTLTextureUsage, NSUInteger);
MTLPP_OPTIONS(MTLPipelineOption, NSUInteger);
MTLPP_OPTIONS(MTLColorWriteMask, NSUInteger);

enum MTLAttributeFormat : NSUInteger;
enum MTLCPUCacheMode : NSUInteger;
enum MTLCommandBufferStatus : NSUInteger;
enum MTLCullMode : NSUInteger;
enum MTLDepthClipMode : NSUInteger;
enum MTLFeatureSet : NSUInteger;
enum MTLIndexType : NSUInteger;
enum MTLLoadAction : NSUInteger;
enum MTLPixelFormat : NSUInteger;
enum MTLPrimitiveType : NSUInteger;
enum MTLPurgeableState : NSUInteger;
enum MTLStepFunction : NSUInteger;
enum MTLStoreAction : NSUInteger;
enum MTLStorageMode : NSUInteger;
enum MTLTextureType : NSUInteger;
enum MTLTriangleFillMode : NSUInteger;
enum MTLVisibilityResultMode : NSUInteger;
enum MTLWinding : NSUInteger;
enum MTLPatchType : NSUInteger;
enum MTLFunctionType : NSUInteger;
enum MTLReadWriteTextureTier : NSUInteger;
enum MTLArgumentBuffersTier : NSUInteger;
enum MTLSamplerMinMagFilter : NSUInteger;
enum MTLSamplerMipFilter : NSUInteger;
enum MTLSamplerAddressMode : NSUInteger;
enum MTLSamplerBorderColor : NSUInteger;
enum MTLCompareFunction : NSUInteger;
enum MTLStencilOperation : NSUInteger;
enum MTLBlendFactor : NSUInteger;
enum MTLBlendOperation : NSUInteger;
enum MTLPrimitiveTopologyClass : NSUInteger;
enum MTLTessellationPartitionMode : NSUInteger;
enum MTLTessellationFactorFormat : NSUInteger;
enum MTLTessellationControlPointIndexType : NSUInteger;
enum MTLTessellationFactorStepFunction : NSUInteger;
enum MTLVertexStepFunction : NSUInteger;
enum MTLVertexFormat : NSUInteger;
enum MTLMultisampleDepthResolveFilter : NSUInteger;
enum MTLDispatchType : NSUInteger;

struct MTLPPOrigin
{
	NSUInteger x, y, z;
};

struct MTLPPSize
{
	NSUInteger width, height, depth;
};

struct MTLPPRegion
{
	MTLPPOrigin origin;
	MTLPPSize   size;
};

struct MTLPPSamplePosition
{
	float x, y;
};

struct MTLPPScissorRect
{
	NSUInteger x, y, width, height;
};

struct MTLPPViewport
{
	double originX, originY, width, height, znear, zfar;
};

struct MTLPPSizeAndAlign
{
	NSUInteger size;
	NSUInteger align;
};

struct MTLPPClearColor
{
	double red;
	double green;
	double blue;
	double alpha;
};

struct _NSRange;
typedef struct _NSRange NSRange;

typedef void (^MTLCommandBufferHandler)(id <MTLCommandBuffer>);
typedef MTLRenderPipelineReflection * MTLAutoreleasedRenderPipelineReflection;
typedef MTLComputePipelineReflection * MTLAutoreleasedComputePipelineReflection;
typedef void (^MTLNewLibraryCompletionHandler)(id <MTLLibrary> library, NSError * error);
typedef void (^MTLNewRenderPipelineStateCompletionHandler)(id <MTLRenderPipelineState> renderPipelineState, NSError * error);
typedef void (^MTLNewRenderPipelineStateWithReflectionCompletionHandler)(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error);
typedef void (^MTLNewComputePipelineStateCompletionHandler)(id <MTLComputePipelineState> computePipelineState, NSError * error);
typedef void (^MTLNewComputePipelineStateWithReflectionCompletionHandler)(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error);
typedef void (^MTLDrawablePresentedHandler)(id<MTLDrawable>);

typedef double CFTimeInterval;

struct CGSize;

typedef struct CGColorSpace * CGColorSpaceRef;

typedef MTLArgument * MTLAutoreleasedArgument;

enum objc_AssociationPolicy : uintptr_t;

MTLPP_EXTERN const void* CFRetain(const void* cf);
MTLPP_EXTERN void CFRelease(const void* cf);
MTLPP_EXTERN Protocol* objc_getProtocol(const char* name);
MTLPP_EXTERN Class objc_lookUpClass(const char* name);
MTLPP_EXTERN Class object_getClass(id obj);
MTLPP_EXTERN Class objc_getRequiredClass(const char * name);
MTLPP_EXTERN Class class_getSuperclass(Class cls);
MTLPP_EXTERN Method class_getInstanceMethod(Class cls, SEL name);
MTLPP_EXTERN BOOL class_conformsToProtocol(Class cls, Protocol * protocol);
MTLPP_EXTERN IMP class_getMethodImplementation(Class cls, SEL name);
MTLPP_EXTERN BOOL class_addMethod(Class cls, SEL name, IMP imp, const char * types);
MTLPP_EXTERN IMP class_replaceMethod(Class cls, SEL name, IMP imp, const char * types);
MTLPP_EXTERN BOOL class_respondsToSelector(Class cls, SEL sel);
MTLPP_EXTERN const char * method_getTypeEncoding(Method m);
MTLPP_EXTERN id objc_getAssociatedObject(id object, const void * key);
MTLPP_EXTERN void objc_setAssociatedObject(id object, const void * key, id value, objc_AssociationPolicy policy);

namespace mtlpp
{
	enum ResourceUsage : NSUInteger;
}

MTLPP_END

#endif /* declare_h */
