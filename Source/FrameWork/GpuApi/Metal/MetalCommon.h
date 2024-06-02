#pragma once
#import <Metal/Metal.h>

#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>

#ifndef GENERATE_XCODE_GPUTRACE
#define GENERATE_XCODE_GPUTRACE 0
#endif



DECLARE_LOG_CATEGORY_EXTERN(LogMetal, Log, All);
inline DEFINE_LOG_CATEGORY(LogMetal);

typedef NS::SharedPtr<MTL::CommandBuffer> MTLCommandBufferPtr;
typedef NS::SharedPtr<MTL::Texture> MTLTexturePtr;
typedef NS::SharedPtr<MTL::Buffer> MTLBufferPtr;
typedef NS::SharedPtr<MTL::Heap> MTLHeapPtr;

typedef NS::SharedPtr<MTL::TextureDescriptor> MTLTextureDescriptorPtr;
typedef NS::SharedPtr<MTL::VertexDescriptor> MTLVertexDescriptorPtr;
typedef NS::SharedPtr<MTL::RenderCommandEncoder> MTLRenderCommandEncoderPtr;
typedef NS::SharedPtr<MTL::ComputeCommandEncoder> MTLComputeCommandEncoderPtr;
typedef NS::SharedPtr<MTL::BlitCommandEncoder> MTLBlitCommandEncoderPtr;
typedef NS::SharedPtr<MTL::AccelerationStructureCommandEncoder> MTLAccelerationStructureCommandEncoderPtr;
typedef NS::SharedPtr<MTL::RenderPipelineDescriptor> MTLRenderPipelineDescriptorPtr;
typedef NS::SharedPtr<MTL::MeshRenderPipelineDescriptor> MTLMeshRenderPipelineDescriptorPtr;
typedef NS::SharedPtr<MTL::ComputePipelineDescriptor> MTLComputePipelineDescriptorPtr;
typedef NS::SharedPtr<MTL::TileRenderPipelineDescriptor> MTLTileRenderPipelineDescriptorPtr;

typedef NS::SharedPtr<MTL::RenderPipelineState> MTLRenderPipelineStatePtr;
typedef NS::SharedPtr<MTL::ComputePipelineState> MTLComputePipelineStatePtr;

typedef NS::SharedPtr<MTL::RenderPipelineReflection> MTLRenderPipelineReflectionPtr;
typedef NS::SharedPtr<MTL::ComputePipelineReflection> MTLComputePipelineReflectionPtr;

typedef NS::SharedPtr<MTL::Library> MTLLibraryPtr;
typedef NS::SharedPtr<MTL::Function> MTLFunctionPtr;

typedef NS::SharedPtr<MTL::RenderPassDescriptor> MTLRenderPassDescriptorPtr;
typedef NS::SharedPtr<MTL::ArgumentEncoder> MTLArgumentEncoderPtr;
typedef NS::SharedPtr<MTL::SamplerState> MTLSamplerStatePtr;

extern FString NSStringToFString(NS::String* InputString);
extern NS::String* FStringToNSString(const FString& InputString);
