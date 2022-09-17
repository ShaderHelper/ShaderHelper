// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"
#include "Math/Vector2D.h"
#include "Templates/SharedPointer.h"
#include "Misc/Optional.h"

class FGenericWindow;

namespace EMouseButtons
{
	enum Type
	{
		Left = 0,
		Middle,
		Right,
		Thumb01,
		Thumb02,

		Invalid,
	};
}

struct APPLICATIONCORE_API FGamepadKeyNames
{
	typedef FName Type;

	static const FName Invalid;

	static const FName LeftAnalogX;
	static const FName LeftAnalogY;
	static const FName RightAnalogX;
	static const FName RightAnalogY;
	static const FName LeftTriggerAnalog;
	static const FName RightTriggerAnalog;

	static const FName LeftThumb;
	static const FName RightThumb;
	static const FName SpecialLeft;
	static const FName SpecialLeft_X;
	static const FName SpecialLeft_Y;
	static const FName SpecialRight;
	static const FName FaceButtonBottom;
	static const FName FaceButtonRight;
	static const FName FaceButtonLeft;
	static const FName FaceButtonTop;
	static const FName LeftShoulder;
	static const FName RightShoulder;
	static const FName LeftTriggerThreshold;
	static const FName RightTriggerThreshold;
	static const FName DPadUp;
	static const FName DPadDown;
	static const FName DPadRight;
	static const FName DPadLeft;

	static const FName LeftStickUp;
	static const FName LeftStickDown;
	static const FName LeftStickRight;
	static const FName LeftStickLeft;

	static const FName RightStickUp;
	static const FName RightStickDown;
	static const FName RightStickRight;
	static const FName RightStickLeft;
};

enum class EWindowActivation : uint8
{
	Activate,
	ActivateByMouse,
	Deactivate
};

namespace EWindowZone
{
	/**
	 * The Window Zone is the window area we are currently over to send back to the operating system
	 * for operating system compliance.
	 */
	enum Type
	{
		NotInWindow			= 0,
		TopLeftBorder		= 1,
		TopBorder			= 2,
		TopRightBorder		= 3,
		LeftBorder			= 4,
		ClientArea			= 5,
		RightBorder			= 6,
		BottomLeftBorder	= 7,
		BottomBorder		= 8,
		BottomRightBorder	= 9,
		TitleBar			= 10,
		MinimizeButton		= 11,
		MaximizeButton		= 12,
		CloseButton			= 13,
		SysMenu				= 14,

		/** No zone specified */
		Unspecified	= 0,
	};
}


namespace EWindowAction
{
	enum Type
	{
		ClickedNonClientArea	= 1,
		Maximize				= 2,
		Restore					= 3,
		WindowMenu				= 4,
	};
}


/**
 * 
 */
namespace EDropEffect
{
	enum Type
	{
		None = 0,
		Copy = 1,
		Move = 2,
		Link = 3,
	};
}


enum class EGestureEvent : uint8
{
	None,
	Scroll,
	Magnify,
	Swipe,
	Rotate,
	LongPress,
	Count
};


/** Defines the minimum and maximum dimensions that a window can take on. */
struct FWindowSizeLimits
{
public:
	FWindowSizeLimits& SetMinWidth(TOptional<float> InValue){ MinWidth = InValue; return *this; }
	const TOptional<float>& GetMinWidth() const { return MinWidth; }

	FWindowSizeLimits& SetMinHeight(TOptional<float> InValue){ MinHeight = InValue; return *this; }
	const TOptional<float>& GetMinHeight() const { return MinHeight; }

	FWindowSizeLimits& SetMaxWidth(TOptional<float> InValue){ MaxWidth = InValue; return *this; }
	const TOptional<float>& GetMaxWidth() const { return MaxWidth; }

	FWindowSizeLimits& SetMaxHeight(TOptional<float> InValue){ MaxHeight = InValue; return *this; }
	const TOptional<float>& GetMaxHeight() const { return MaxHeight; }

private:
	TOptional<float> MinWidth;
	TOptional<float> MinHeight;
	TOptional<float> MaxWidth;
	TOptional<float> MaxHeight;
};

/** 
 * Context scope that indicates which IInputDevice is currently being handled. 
 * This can be used to determine hardware-specific information when handling input from FGenericApplicationMessageHandler subclasses.
 * This is generally set during SendControllerEvents or Tick and is only valid on the game thread.
 */
class APPLICATIONCORE_API FInputDeviceScope
{
public:
	/** The specific InputDevice that is currently being polled. This is only valid within the current function scope and may be null */
	class IInputDevice* InputDevice;

	/** Logical name of the input device interface. This is not translated but is platform-specific */
	FName InputDeviceName;

	/** A system-specific device id, this is not the same as controllerId and represents a physical device instead of logical user. -1 represents an unknown device */
	int32 HardwareDeviceHandle;

	/** Logical string identifying the hardware device. This is not translated and is system-specific, it may be empty */
	FString HardwareDeviceIdentifier;

	/** Constructor, this should only be allocated directly on the stack */
	FInputDeviceScope(IInputDevice* InInputDevice, FName InInputDeviceName, int32 InHardwareDeviceHandle = -1, FString InHardwareDeviceIdentifier = FString());
	~FInputDeviceScope();

	/** Cannot be copied/moved */
	FInputDeviceScope() = delete;
	FInputDeviceScope(const FInputDeviceScope&) = delete;
	FInputDeviceScope& operator=(const FInputDeviceScope&) = delete;
	FInputDeviceScope(FInputDeviceScope&&) = delete;
	FInputDeviceScope& operator=(FInputDeviceScope&&) = delete;

	/** Returns the currently active InputDeviceScope. This is only valid to call on the game thread and may return null */
	static const FInputDeviceScope* GetCurrent();

private:
	static TArray<FInputDeviceScope*> ScopeStack;
};

PRAGMA_DISABLE_DEPRECATION_WARNINGS

/** Interface that defines how to handle interaction with a user via hardware input and output */
class FGenericApplicationMessageHandler
{
public:

	virtual ~FGenericApplicationMessageHandler() {}

	virtual bool ShouldProcessUserInputMessages( const TSharedPtr< FGenericWindow >& PlatformWindow ) const
	{
		return false;
	}

	virtual bool OnKeyChar( const TCHAR Character, const bool IsRepeat )
	{
		return false;
	}

	virtual bool OnKeyDown( const int32 KeyCode, const uint32 CharacterCode, const bool IsRepeat ) 
	{
		return false;
	}

	virtual bool OnKeyUp( const int32 KeyCode, const uint32 CharacterCode, const bool IsRepeat )
	{
		return false;
	}

	virtual void OnInputLanguageChanged()
	{
	}

	virtual bool OnMouseDown( const TSharedPtr< FGenericWindow >& Window, const EMouseButtons::Type Button )
	{
		return false;
	}

	virtual bool OnMouseDown( const TSharedPtr< FGenericWindow >& Window, const EMouseButtons::Type Button, const FVector2D CursorPos )
	{
		return false;
	}

	virtual bool OnMouseUp( const EMouseButtons::Type Button )
	{
		return false;
	}

	virtual bool OnMouseUp( const EMouseButtons::Type Button, const FVector2D CursorPos )
	{
		return false;
	}

	virtual bool OnMouseDoubleClick( const TSharedPtr< FGenericWindow >& Window, const EMouseButtons::Type Button )
	{
		return false;
	}

	virtual bool OnMouseDoubleClick( const TSharedPtr< FGenericWindow >& Window, const EMouseButtons::Type Button, const FVector2D CursorPos )
	{
		return false;
	}

	virtual bool OnMouseWheel( const float Delta )
	{
		return false;
	}

	virtual bool OnMouseWheel( const float Delta, const FVector2D CursorPos )
	{
		return false;
	}

	virtual bool OnMouseMove()
	{
		return false;
	}

	virtual bool OnRawMouseMove( const int32 X, const int32 Y )
	{
		return false;
	}

	virtual bool OnCursorSet()
	{
		return false;
	}

	/** 
	 * Return true if this message handler expects FPlatformUserIds. This base class will convert both directions.
	 * As part of a larger fixup to allow mapping of multiple input devices to the same player, 
	 * physical device id will be passed as part of the InputScope above and used to compute a logical input user.
	 */
	virtual bool ShouldUsePlatformUserId() const
	{
		return false;
	}

	virtual bool OnControllerAnalog(FGamepadKeyNames::Type KeyName, FPlatformUserId PlatformUserId, float AnalogValue)
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnControllerAnalog(KeyName, PlatformUserId.GetInternalId(), AnalogValue);
		}
		return false;
	}

	virtual bool OnControllerButtonPressed(FGamepadKeyNames::Type KeyName, FPlatformUserId PlatformUserId, bool IsRepeat)
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnControllerButtonPressed(KeyName, PlatformUserId.GetInternalId(), IsRepeat);
		}
		return false;
	}

	virtual bool OnControllerButtonReleased(FGamepadKeyNames::Type KeyName, FPlatformUserId PlatformUserId, bool IsRepeat)
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnControllerButtonReleased(KeyName, PlatformUserId.GetInternalId(), IsRepeat);
		}
		return false;
	}

    virtual void OnBeginGesture()
    {
    }

	virtual bool OnTouchGesture( EGestureEvent GestureType, const FVector2D& Delta, float WheelDelta, bool bIsDirectionInvertedFromDevice )
	{
		return false;
	}
    
    virtual void OnEndGesture()
    {
    }

	virtual bool OnTouchStarted( const TSharedPtr< FGenericWindow >& Window, const FVector2D& Location, float Force, int32 TouchIndex, FPlatformUserId PlatformUserId )
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnTouchStarted(Window, Location, Force, TouchIndex, PlatformUserId.GetInternalId());
		}
		return false;
	}

	virtual bool OnTouchMoved( const FVector2D& Location, float Force, int32 TouchIndex, FPlatformUserId PlatformUserId )
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnTouchMoved(Location, Force, TouchIndex, PlatformUserId.GetInternalId());
		}
		return false;
	}

	virtual bool OnTouchEnded( const FVector2D& Location, int32 TouchIndex, FPlatformUserId PlatformUserId )
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnTouchEnded(Location, TouchIndex, PlatformUserId.GetInternalId());
		}
		return false;
	}

	virtual bool OnTouchForceChanged(const FVector2D& Location, float Force, int32 TouchIndex, FPlatformUserId PlatformUserId)
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnTouchForceChanged(Location, Force, TouchIndex, PlatformUserId.GetInternalId());
		}
		return false;
	}

	virtual bool OnTouchFirstMove(const FVector2D& Location, float Force, int32 TouchIndex, FPlatformUserId PlatformUserId)
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnTouchFirstMove(Location, Force, TouchIndex, PlatformUserId.GetInternalId());
		}
		return false;
	}

	virtual void ShouldSimulateGesture(EGestureEvent Gesture, bool bEnable)
	{

	}

	virtual bool OnMotionDetected( const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration, FPlatformUserId PlatformUserId )
	{
		if (!ShouldUsePlatformUserId())
		{
			return OnMotionDetected(Tilt, RotationRate, Gravity, Acceleration, PlatformUserId.GetInternalId());
		}
		return false;
	}

	virtual bool OnSizeChanged( const TSharedRef< FGenericWindow >& Window, const int32 Width, const int32 Height, bool bWasMinimized = false )
	{
		return false;
	}

	virtual void OnOSPaint( const TSharedRef<FGenericWindow>& Window )
	{
	
	}

	virtual FWindowSizeLimits GetSizeLimitsForWindow( const TSharedRef<FGenericWindow>& Window ) const
	{
		return FWindowSizeLimits();
	}

	virtual void OnResizingWindow( const TSharedRef< FGenericWindow >& Window )
	{

	}

	virtual bool BeginReshapingWindow( const TSharedRef< FGenericWindow >& Window )
	{
		return true;
	}

	virtual void FinishedReshapingWindow( const TSharedRef< FGenericWindow >& Window )
	{

	}

	virtual void HandleDPIScaleChanged( const TSharedRef< FGenericWindow >& Window )
	{

	}

	virtual void SignalSystemDPIChanged(const TSharedRef< FGenericWindow >& Window)
	{

	}	

	virtual void OnMovedWindow( const TSharedRef< FGenericWindow >& Window, const int32 X, const int32 Y )
	{

	}

	virtual bool OnWindowActivationChanged( const TSharedRef< FGenericWindow >& Window, const EWindowActivation ActivationType )
	{
		return false;
	}

	virtual bool OnApplicationActivationChanged( const bool IsActive )
	{
		return false;
	}

	virtual bool OnConvertibleLaptopModeChanged()
	{
		return false;
	}

	virtual EWindowZone::Type GetWindowZoneForPoint( const TSharedRef< FGenericWindow >& Window, const int32 X, const int32 Y )
	{
		return EWindowZone::NotInWindow;
	}

	virtual void OnWindowClose( const TSharedRef< FGenericWindow >& Window )
	{

	}

	virtual EDropEffect::Type OnDragEnterText( const TSharedRef< FGenericWindow >& Window, const FString& Text )
	{
		return EDropEffect::None;
	}

	virtual EDropEffect::Type OnDragEnterFiles( const TSharedRef< FGenericWindow >& Window, const TArray< FString >& Files )
	{
		return EDropEffect::None;
	}

	virtual EDropEffect::Type OnDragEnterExternal( const TSharedRef< FGenericWindow >& Window, const FString& Text, const TArray< FString >& Files )
	{
		return EDropEffect::None;
	}

	virtual EDropEffect::Type OnDragOver( const TSharedPtr< FGenericWindow >& Window )
	{
		return EDropEffect::None;
	}

	virtual void OnDragLeave( const TSharedPtr< FGenericWindow >& Window )
	{

	}

	virtual EDropEffect::Type OnDragDrop( const TSharedPtr< FGenericWindow >& Window )
	{
		return EDropEffect::None;
	}

	virtual bool OnWindowAction( const TSharedRef< FGenericWindow >& Window, const EWindowAction::Type InActionType)
	{
		return true;
	}

	virtual void SetCursorPos(const FVector2D& MouseCoordinate)
	{

	}

	// Deprecate these when engine code has been converted to handle platform user id
	virtual bool OnControllerAnalog(FGamepadKeyNames::Type KeyName, int32 ControllerId, float AnalogValue)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnControllerAnalog(KeyName, FPlatformUserId(ControllerId), AnalogValue);
		}
		return false;
	}
	virtual bool OnControllerButtonPressed(FGamepadKeyNames::Type KeyName, int32 ControllerId, bool IsRepeat)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnControllerButtonPressed(KeyName, FPlatformUserId(ControllerId), IsRepeat);
		}
		return false;
	}
	virtual bool OnControllerButtonReleased(FGamepadKeyNames::Type KeyName, int32 ControllerId, bool IsRepeat)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnControllerButtonReleased(KeyName, FPlatformUserId(ControllerId), IsRepeat);
		}
		return false;
	}
	virtual bool OnTouchStarted(const TSharedPtr< FGenericWindow >& Window, const FVector2D& Location, float Force, int32 TouchIndex, int32 ControllerId)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnTouchStarted(Window, Location, Force, TouchIndex, FPlatformUserId(ControllerId));
		}
		return false;
	}
	virtual bool OnTouchMoved(const FVector2D& Location, float Force, int32 TouchIndex, int32 ControllerId)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnTouchMoved(Location, Force, TouchIndex, FPlatformUserId(ControllerId));
		}
		return false;
	}
	virtual bool OnTouchEnded(const FVector2D& Location, int32 TouchIndex, int32 ControllerId)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnTouchEnded(Location, TouchIndex, FPlatformUserId(ControllerId));
		}
		return false;
	}
	virtual bool OnTouchForceChanged(const FVector2D& Location, float Force, int32 TouchIndex, int32 ControllerId)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnTouchForceChanged(Location, Force, TouchIndex, FPlatformUserId(ControllerId));
		}
		return false;
	}
	virtual bool OnTouchFirstMove(const FVector2D& Location, float Force, int32 TouchIndex, int32 ControllerId)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnTouchFirstMove(Location, Force, TouchIndex, FPlatformUserId(ControllerId));
		}
		return false;
	}
	virtual bool OnMotionDetected(const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration, int32 ControllerId)
	{
		if (ShouldUsePlatformUserId())
		{
			return OnMotionDetected(Tilt, RotationRate, Gravity, Acceleration, FPlatformUserId(ControllerId));
		}
		return false;
	}
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS