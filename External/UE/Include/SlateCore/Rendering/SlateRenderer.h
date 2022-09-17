// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Textures/SlateShaderResource.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Rendering/DrawElements.h"
#include "Templates/RefCounting.h"
#include "Fonts/FontTypes.h"
#include "PixelFormat.h"

class FRHITexture2D;
class FRenderTarget;
class FSlateDrawBuffer;
class FSlateUpdatableTexture;
class ISlate3DRenderer;
class ISlateAtlasProvider;
class ISlateStyle;
class SWindow;
struct Rect;
class FSceneInterface;
struct FSlateBrush;

typedef TRefCountPtr<FRHITexture2D> FTexture2DRHIRef;

/**
 * Update context for deferred drawing of widgets to render targets
 */
struct FRenderThreadUpdateContext
{
	class FSlateDrawBuffer* WindowDrawBuffer;
	float WorldTimeSeconds;
	float DeltaTimeSeconds;
	float RealTimeSeconds;
	float DeltaRealTimeSeconds;
	FRenderTarget* RenderTarget;
	ISlate3DRenderer* Renderer;
	bool bClearTarget;
};

/**
 * Provides access to the game and render thread font caches that Slate should use
 */
class SLATECORE_API FSlateFontServices
{
public:
	/**
	 * Construct the font services from the font caches (we'll create corresponding measure services ourselves)
	 * These pointers may be the same if your renderer doesn't need a separate render thread font cache
	 */
	FSlateFontServices(TSharedRef<class FSlateFontCache> InGameThreadFontCache, TSharedRef<class FSlateFontCache> InRenderThreadFontCache);

	/**
	 * Destruct the font services
	 */
	~FSlateFontServices();

	/**
	 * Get the font cache to use for the current thread
	 */
	TSharedRef<class FSlateFontCache> GetFontCache() const;

	/**
	 * Get the font cache to use for the game thread
	 */
	TSharedRef<class FSlateFontCache> GetGameThreadFontCache() const
	{
		return GameThreadFontCache;
	}

	/**
	 * Get the font cache to use for the render thread
	 */
	TSharedRef<class FSlateFontCache> GetRenderThreadFontCache() const
	{
		return RenderThreadFontCache;
	}

	/**
	 * Get access to the font measure service for the current thread
	 */
	TSharedRef<class FSlateFontMeasure> GetFontMeasureService() const;

	/**
	 * Get access to the font measure service for the current thread
	 */
	TSharedRef<class FSlateFontMeasure> GetGameThreadFontMeasureService() const 
	{
		return GameThreadFontMeasure;
	}

	/**
	 * Get access to the font measure service for the current thread
	 */
	TSharedRef<class FSlateFontMeasure> GetRenderThreadFontMeasureService() const 
	{
		return RenderThreadFontMeasure;
	}

	/**
	 * Flushes all cached data from the font cache for the current thread
	 */
	void FlushFontCache(const FString& FlushReason);

	/**
	 * Flushes all cached data from the font cache for the game thread
	 */
	void FlushGameThreadFontCache(const FString& FlushReason);

	/**
	 * Flushes all cached data from the font cache for the render thread
	 */
	void FlushRenderThreadFontCache(const FString& FlushReason);

	/**
	 * Release any rendering resources owned by this font service
	 */
	void ReleaseResources();

	/**
	 * Delegate called after releasing the rendering resources used by this font service
	 */
	FOnReleaseFontResources& OnReleaseResources();

private:
	void HandleFontCacheReleaseResources(const class FSlateFontCache& InFontCache);

	TSharedRef<class FSlateFontCache> GameThreadFontCache;
	TSharedRef<class FSlateFontCache> RenderThreadFontCache;

	TSharedRef<class FSlateFontMeasure> GameThreadFontMeasure;
	TSharedRef<class FSlateFontMeasure> RenderThreadFontMeasure;

	FOnReleaseFontResources OnReleaseResourcesDelegate;
};


struct FMappedTextureBuffer
{
	void* Data;
	int32 Width;
	int32 Height;

	FMappedTextureBuffer()
		: Data(nullptr)
		, Width(0)
		, Height(0)
	{
	}

	bool IsValid() const
	{
		return Data && Width > 0 && Height > 0;
	}

	void Reset()
	{
		Data = nullptr;
		Width = Height = 0;
	}
};


/**
 * Abstract base class for Slate renderers.
 */
class SLATECORE_API FSlateRenderer
{
public:

	/** Constructor. */
	explicit FSlateRenderer(const TSharedRef<FSlateFontServices>& InSlateFontServices);

	/** Virtual destructor. */
	virtual ~FSlateRenderer();

public:

	/** Returns a draw buffer that can be used by Slate windows to draw window elements */
	virtual FSlateDrawBuffer& GetDrawBuffer() = 0;

	virtual bool Initialize() = 0;

	virtual void Destroy() = 0;

	/**
	 * Creates a rendering viewport
	 *
	 * @param InWindow	The window to create the viewport for
	 */ 
	virtual void CreateViewport( const TSharedRef<SWindow> InWindow ) = 0;

	/**
	 * Requests that a rendering viewport be resized
	 *
	 * @param Window		The window to resize
	 * @param Width			The new width of the window
	 * @param Height		The new height of the window
	 */
	virtual void RequestResize( const TSharedPtr<SWindow>& InWindow, uint32 NewSizeX, uint32 NewSizeY ) = 0;

	/**
	 * Sets fullscreen state on the window's rendering viewport
	 *
	 * @param	InWindow		The window to update fullscreen state on
	 * @param	OverrideResX	0 if no override
	 * @param	OverrideResY	0 if no override
	 */
	virtual void UpdateFullscreenState( const TSharedRef<SWindow> InWindow, uint32 OverrideResX = 0, uint32 OverrideResY = 0 ) = 0;


	/**
	 * Set the resolution cached by the engine
	 *
	 * @param	Width			Width of the system resolution
	 * @param	Height			Height of the system resolution
	 */
	virtual void SetSystemResolution(uint32 Width, uint32 Height) = 0;
	
	
	/**
	 * Restore the given window to the resolution settings currently cached by the engine
	 * 
	 * @param InWindow    -> The window to restore to the cached settings
	 */
	virtual void RestoreSystemResolution(const TSharedRef<SWindow> InWindow) = 0;

	/** 
	 * Creates necessary resources to render a window and sends draw commands to the rendering thread
	 *
	 * @param WindowDrawBuffer	The buffer containing elements to draw 
	 */
	virtual void DrawWindows( FSlateDrawBuffer& InWindowDrawBuffer ) = 0;
	
	/** Callback that fires after Slate has rendered each window, each frame */
	DECLARE_MULTICAST_DELEGATE_TwoParams( FOnSlateWindowRendered, SWindow&, void* );
	FOnSlateWindowRendered& OnSlateWindowRendered() { return SlateWindowRendered; }

	/**
	 * Called on the game thread right before the slate window handle is destroyed.  
	 * This gives users a chance to release any viewport specific resources they may have active when the window is destroyed 
	 * @param Pointer to the API specific backbuffer type
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlateWindowDestroyed, void*);
	FOnSlateWindowDestroyed& OnSlateWindowDestroyed() { return OnSlateWindowDestroyedDelegate; }

	/**
	 * Called on the game thread right before a window backbuffer is about to be resized
	 * @param Pointer to the API specific backbuffer type
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreResizeWindowBackbuffer, void*);
	FOnPreResizeWindowBackbuffer& OnPreResizeWindowBackBuffer() { return PreResizeBackBufferDelegate; }

	/**
	 * Called on the game thread right after a window backbuffer has been resized
	 * @param Pointer to the API specific backbuffer type
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPostResizeWindowBackbuffer, void*);
	FOnPostResizeWindowBackbuffer& OnPostResizeWindowBackBuffer() { return PostResizeBackBufferDelegate; }

	/** Callback on the render thread after slate rendering finishes and right before present is called */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBackBufferReadyToPresent, SWindow&, const FTexture2DRHIRef&);
	FOnBackBufferReadyToPresent& OnBackBufferReadyToPresent() { return OnBackBufferReadyToPresentDelegate; }

	/** 
	 * Sets which color vision filter to use
	 */
	virtual void SetColorVisionDeficiencyType(EColorVisionDeficiency Type, int32 Severity, bool bCorrectDeficiency, bool bShowCorrectionWithDeficiency) { }

	/** 
	 * Creates a dynamic image resource and returns its size
	 *
	 * @param InTextureName The name of the texture resource
	 * @return The size of the loaded texture
	 */
	virtual FIntPoint GenerateDynamicImageResource(const FName InTextureName) {check(0); return FIntPoint( 0, 0 );}

	/**
	 * Creates a dynamic image resource
	 *
	 * @param ResourceName		The name of the texture resource
	 * @param Width				The width of the resource
	 * @param Height			The height of the image
	 * @param Bytes				The payload for the resource
	 * @return					true if the resource was successfully generated, otherwise false
	 */
	virtual bool GenerateDynamicImageResource( FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes ) { return false; }

	virtual bool GenerateDynamicImageResource(FName ResourceName, FSlateTextureDataRef TextureData) { return false; }

	/**
	 * Creates a resource handle with a specific size and scale (for brushes with dynamic resizing such as vector art)
	 * A handle is used as fast path for looking up a rendering resource for a given brush when adding Slate draw elements
	 * This can be cached and stored safely in code.  It will become invalid when a resource is destroyed
	 * It is expensive to create a resource so do not do it in time sensitive areas
	 *
	 * @param	Brush		The brush to get a rendering resource handle
	 * @param	LocalSize	The local size of the element using the brush (Vector graphics only)
	 * @param	DrawScale	The draw scale of the element using the brush (Vector graphics only)
	 * @return	The created resource handle.
	 */
	virtual FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush, FVector2D LocalSize, float DrawScale) = 0;

	/**
	 * Creates a handle to a Slate resource
	 * A handle is used as fast path for looking up a rendering resource for a given brush when adding Slate draw elements
	 * This can be cached and stored safely in code.  It will become invalid when a resource is destroyed
	 * It is expensive to create a resource so do not do it in time sensitive areas
	 *
	 * @param	Brush		The brush to get a rendering resource handle 
	 * @return	The created resource handle.  
	 */
	virtual FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush)
	{
		return GetResourceHandle(Brush, FVector2D::ZeroVector, 1.0f);
	}


	/** The default implementation assumes all things are renderable. */
	virtual bool CanRenderResource(UObject& InResourceObject) const { return true; }

	/**
	 * Queues a dynamic brush for removal when it is safe.  The brush is not immediately released but you should consider the brush destroyed and no longer usable
	 *
	 * @param BrushToRemove	The brush to queue for removal which is no longer valid to use
	 */
	virtual void RemoveDynamicBrushResource( TSharedPtr<FSlateDynamicImageBrush> BrushToRemove ) = 0;

	/** 
	 * Releases a specific resource.  
	 * It is unlikely that you want to call this directly.  Use RemoveDynamicBrushResource instead
	 */
	virtual void ReleaseDynamicResource( const FSlateBrush& Brush ) = 0;


	/** Called when a window is destroyed to give the renderer a chance to free resources */
	virtual void OnWindowDestroyed( const TSharedRef<SWindow>& InWindow ) = 0;
	
	/** Called when a window is finished being reshaped */
	virtual void OnWindowFinishReshaped(const TSharedPtr<SWindow>& InWindow) {};

	/**
	 * Returns the viewport rendering resource (backbuffer) for the provided window
	 *
	 * @param Window	The window to get the viewport from 
	 */
	virtual void* GetViewportResource( const SWindow& Window ) { return nullptr; }
	
	/**
	 * Get access to the font services used by this renderer
	 */
	TSharedRef<FSlateFontServices> GetFontServices() const 
	{
		return SlateFontServices.ToSharedRef();
	}

	/**
	 * Get access to the font measure service (game thread only!)
	 */
	TSharedRef<class FSlateFontMeasure> GetFontMeasureService() const 
	{
		return SlateFontServices->GetFontMeasureService();
	}

	/**
	 * Get the font cache to use for the current thread
	 */
	TSharedRef<class FSlateFontCache> GetFontCache() const
	{
		return SlateFontServices->GetFontCache();
	}

	/**
	 * Flushes all cached data from the font cache for the current thread
	 */
	void FlushFontCache(const FString& FlushReason)
	{
		SlateFontServices->FlushFontCache(FlushReason);
	}

	/**
	 * Gives the renderer a chance to wait for any render commands to be completed before returning/
	 */
	virtual void FlushCommands() const {};

	/**
	 * Gives the renderer a chance to synchronize with another thread in the event that the renderer runs 
	 * in a multi-threaded environment.  This function does not return until the sync is complete
	 */
	virtual void Sync() const {};
	
	/**
	 * Indicates the start of a new frame to the Renderer. This is usually handled by the engine loop
	 * but certain situations (ie, when the main loop is paused) may require manual calls.
	 */
	virtual void BeginFrame() const {};
	
	/**
	 * Indicates the end of the current frame to the Renderer. This is usually handled by the engine loop
	 * but certain situations (ie, when the main loop is paused) may require manual calls.
	 */
	virtual void EndFrame() const {};

	/**
	 * Reloads all texture resources from disk
	 */
	virtual void ReloadTextureResources() {}

	/**
	 * Loads all the resources used by the specified SlateStyle
	 */
	virtual void LoadStyleResources( const ISlateStyle& Style ) {}

	/**
	 * Returns whether or not a viewport should be in  fullscreen
	 *
	 * @Window	The window to check for fullscreen
	 * @return true if the window's viewport should be fullscreen
	 */
	bool IsViewportFullscreen( const SWindow& Window ) const;

	/** Returns whether shaders that Slate depends on have been compiled. */
	virtual bool AreShadersInitialized() const { return true; }

	/** 
	 * Removes references to FViewportRHI's.  
	 * This has to be done explicitly instead of using the FRenderResource mechanism because FViewportRHI's are managed by the game thread.
	 * This is needed before destroying the RHI device. 
	 */
	virtual void InvalidateAllViewports() {}

	/**
	 * A renderer may need to keep a cache of accessed garbage collectible objects alive for the duration of their
	 * usage.  During some operations like ending a game.  It becomes important to immediately release game related
	 * resources.  This should flush any buffer holding onto those referenced objects.
	 */
	virtual void ReleaseAccessedResources(bool bImmediatelyFlush) {}

	/** 
	 * Prepares the renderer to take a screenshot of the UI.  The Rect is portion of the rendered output
	 * that will be stored into the TArray of FColors.
	 */
	virtual void PrepareToTakeScreenshot(const FIntRect& Rect, TArray<FColor>* OutColorData, SWindow* InScreenshotWindow) {}

	/**
	 * Pushes the rendering of the specified window to the specified render target
	 */
	virtual void SetWindowRenderTarget(const SWindow& Window, class IViewportRenderTargetProvider* Provider) {}

	/**
	 * Create an updatable texture that can receive new data dynamically
	 *
	 * @param	Width	Initial width of the texture
	 * @param	Height	Initial height of the texture
	 *
	 * @return	Newly created updatable texture
	 */
	virtual FSlateUpdatableTexture* CreateUpdatableTexture(uint32 Width, uint32 Height) = 0;

	/**
	 * Create an updatable texture that can receive new data via a shared handle
	 *
	 * @param	SharedHandle	The OS dependant handle that backs the texture data
	 *
	 * @return	Newly created updatable texture
	 */
	virtual FSlateUpdatableTexture* CreateSharedHandleTexture(void *SharedHandle) = 0;

	/**
	 * Return an updatable texture to the renderer for release
	 *
	 * @param	Texture	The texture we are releasing (should not use this pointer after calling)
	 */
	virtual void ReleaseUpdatableTexture(FSlateUpdatableTexture* Texture) = 0;

	/**
	 * Returns the way to access the texture atlas information for this renderer
	 */
	virtual ISlateAtlasProvider* GetTextureAtlasProvider();

	/**
	 * Returns the way to access the font atlas information for this renderer
	 */
	virtual ISlateAtlasProvider* GetFontAtlasProvider();

	/**
	 * Copies all slate windows out to a buffer at half resolution with debug information
	 * like the mouse cursor and any keypresses.
	 */
	virtual void CopyWindowsToVirtualScreenBuffer(const TArray<FString>& KeypressBuffer) {}
	
	/** Allows and disallows access to the crash tracker buffer data on the CPU */
	virtual void MapVirtualScreenBuffer(FMappedTextureBuffer* OutImageData) {}
	virtual void UnmapVirtualScreenBuffer() {}

	/**
	 * Necessary to grab before flushing the resource pool, as it may be being 
	 * accessed by multiple threads when loading.
	 */
	virtual FCriticalSection* GetResourceCriticalSection() = 0;

	/** Register the active scene pointer with the renderer. This will return the scene internal index that will be used for all subsequent elements drawn. */
	virtual int32 RegisterCurrentScene(FSceneInterface* Scene) = 0;

	/** Get the currently registered scene index (set by RegisterCurrentScene)*/
	virtual int32 GetCurrentSceneIndex() const  = 0;

	/** Reset the internal Scene tracking.*/
	virtual void ClearScenes() = 0;

	virtual void DestroyCachedFastPathRenderingData(struct FSlateCachedFastPathRenderingData* VertexData);
	virtual void DestroyCachedFastPathElementData(struct FSlateCachedElementData* ElementData);

	virtual bool HasLostDevice() const { return false; }

	/**
	 * Lets the renderer know that we need to render some widgets to a render target.
	 * 
	 * @param Context						The context that describes what we're rendering to
	 * @param bDeferredRenderTargetUpdate	Whether or not the update is deferred until the end of the frame when it is potentially less expensive to update the render target. 
											See GDeferRetainedRenderingRenderThread for more info.
											Care must be taken to destroy anything referenced in the context when it is safe to do so.
	 */
	virtual void AddWidgetRendererUpdate(const struct FRenderThreadUpdateContext& Context, bool bDeferredRenderTargetUpdate) {}

	virtual EPixelFormat GetSlateRecommendedColorFormat() { return PF_B8G8R8A8; }
private:

	// Non-copyable
	FSlateRenderer(const FSlateRenderer&);
	FSlateRenderer& operator=(const FSlateRenderer&);

	void HandleFontCacheReleaseResources(const class FSlateFontCache& InFontCache);

protected:

	/** The font services used by this renderer when drawing text */
	TSharedPtr<FSlateFontServices> SlateFontServices;

	/** Callback that fires after Slate has rendered each window, each frame */
	FOnSlateWindowRendered SlateWindowRendered;

	FOnSlateWindowDestroyed OnSlateWindowDestroyedDelegate;
	FOnPreResizeWindowBackbuffer PreResizeBackBufferDelegate;
	FOnPostResizeWindowBackbuffer PostResizeBackBufferDelegate;

	FOnBackBufferReadyToPresent OnBackBufferReadyToPresentDelegate;

	/**
	 * Necessary to grab before flushing the resource pool, as it may be being 
	 * accessed by multiple threads when loading.
	 */
	FCriticalSection ResourceCriticalSection;

	friend class FSlateRenderDataHandle;
};


/**
 * Is this thread valid for sending out rendering commands?
 * If the slate loading thread exists, then yes, it is always safe
 * Otherwise, we have to be on the game thread
 */
bool SLATECORE_API IsThreadSafeForSlateRendering();

/**
 * If it's the game thread, and there's no loading thread, then it owns slate rendering.
 * However if there's a loading thread, it is the exlusive owner of slate rendering.
 */
bool SLATECORE_API DoesThreadOwnSlateRendering();
