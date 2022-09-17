// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Misc/Attribute.h"
#include "InputCoreTypes.generated.h"

INPUTCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogInput, Log, All);


/** Defines the controller hands for tracking.  Could be expanded, as needed, to facilitate non-handheld controllers */
UENUM(BlueprintType)
enum class EControllerHand : uint8
{
	Left,
	Right,
	AnyHand,
	Pad,
	ExternalCamera,
	Gun,
	HMD,
	Special_1,
	Special_2,
	Special_3,
	Special_4,
	Special_5,
	Special_6,
	Special_7,
	Special_8,
	Special_9,
	Special_10,
	Special_11,

	ControllerHand_Count UMETA(Hidden, DisplayName = "<INVALID>"),
};

enum class EPairedAxis : uint8
{
	Unpaired,			// This key is unpaired
	X,					// This key represents the X axis of its PairedAxisKey
	Y,					// This key represents the Y axis of its PairedAxisKey
	Z,					// This key represents the Z axis of its PairedAxisKey - Currently unused
};

USTRUCT(BlueprintType,Blueprintable)
struct INPUTCORE_API FKey
{
	GENERATED_USTRUCT_BODY()

	FKey()
	{
	}

	FKey(const FName InName)
		: KeyName(InName)
	{
	}

	FKey(const TCHAR* InName)
		: KeyName(FName(InName))
	{
	}

	FKey(const ANSICHAR* InName)
		: KeyName(FName(InName))
	{
	}

	bool IsValid() const;
	bool IsModifierKey() const;
	bool IsGamepadKey() const;
	bool IsTouch() const;
	bool IsMouseButton() const;
	bool IsButtonAxis() const;
	bool IsAxis1D() const;
	bool IsAxis2D() const;
	bool IsAxis3D() const;
	UE_DEPRECATED(4.26, "Please use IsAxis1D instead.")
	bool IsFloatAxis() const;
	UE_DEPRECATED(4.26, "Please use IsAxis2D/IsAxis3D instead.")
	bool IsVectorAxis() const;
	bool IsDigital() const;
	bool IsAnalog() const;
	bool IsBindableInBlueprints() const;
	bool ShouldUpdateAxisWithoutSamples() const;
	bool IsBindableToActions() const;
	bool IsDeprecated() const;
	bool IsGesture() const;
	FText GetDisplayName(bool bLongDisplayName = true) const;
	FString ToString() const;
	FName GetFName() const;
	FName GetMenuCategory() const;
	EPairedAxis GetPairedAxis() const;
	FKey GetPairedAxisKey() const;

	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FStructuredArchive::FSlot Slot);
	bool ExportTextItem(FString& ValueStr, FKey const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	void PostSerialize(const FArchive& Ar);
	void PostScriptConstruct();

	friend bool operator==(const FKey& KeyA, const FKey& KeyB) { return KeyA.KeyName == KeyB.KeyName; }
	friend bool operator!=(const FKey& KeyA, const FKey& KeyB) { return KeyA.KeyName != KeyB.KeyName; }
	friend bool operator<(const FKey& KeyA, const FKey& KeyB) { return KeyA.KeyName.LexicalLess(KeyB.KeyName); }
	friend uint32 GetTypeHash(const FKey& Key) { return GetTypeHash(Key.KeyName); }

	friend struct EKeys;

	static const TCHAR* SyntheticCharPrefix;

private:

	UPROPERTY()
	FName KeyName;

	mutable class TSharedPtr<struct FKeyDetails> KeyDetails;

	void ConditionalLookupKeyDetails() const;
	void ResetKey();
};

template<>
struct TStructOpsTypeTraits<FKey> : public TStructOpsTypeTraitsBase2<FKey>
{
	enum
	{
		WithStructuredSerializeFromMismatchedTag = true,
		WithExportTextItem = true,
		WithImportTextItem = true,
		WithPostSerialize = true,
		WithPostScriptConstruct = true,
		WithCopy = true,		// Necessary so that TSharedPtr<FKeyDetails> Data is copied around
	};
};

DECLARE_DELEGATE_RetVal_OneParam(FText, FGetKeyDisplayNameSignature, const FKey);

struct INPUTCORE_API FKeyDetails
{
	enum EKeyFlags
	{
		GamepadKey				 = 1 << 0,
		Touch					 = 1 << 1,
		MouseButton				 = 1 << 2,
		ModifierKey				 = 1 << 3,
		NotBlueprintBindableKey	 = 1 << 4,
		Axis1D					 = 1 << 5,
		Axis3D					 = 1 << 6,
		UpdateAxisWithoutSamples = 1 << 7,
		NotActionBindableKey	 = 1 << 8,
		Deprecated				 = 1 << 9,

		// All axis representations
		ButtonAxis				 = 1 << 10,		// Analog 1D axis emulating a digital button press. E.g. Gamepad right stick up
		Axis2D					 = 1 << 11,

		// Gesture input types such as Flick, Pinch, and Rotate
		Gesture					 = 1 << 12,

		// Deprecated. Replace with axis definitions for clarity.

		FloatAxis  UE_DEPRECATED(4.26, "Please use Axis1D instead.") = Axis1D,
		VectorAxis UE_DEPRECATED(4.26, "Please use Axis2D/Axis3D instead.") = Axis3D,

		NoFlags                  = 0,
	};

	FKeyDetails(const FKey InKey, const TAttribute<FText>& InLongDisplayName, const uint32 InKeyFlags = 0, const FName InMenuCategory = NAME_None, const TAttribute<FText>& InShortDisplayName = TAttribute<FText>() );
	FKeyDetails(const FKey InKey, const TAttribute<FText>& InLongDisplayName, const TAttribute<FText>& InShortDisplayName, const uint32 InKeyFlags = 0, const FName InMenuCategory = NAME_None);

	FORCEINLINE bool IsModifierKey() const { return bIsModifierKey != 0; }
	FORCEINLINE bool IsGamepadKey() const { return bIsGamepadKey != 0; }
	FORCEINLINE bool IsTouch() const { return bIsTouch != 0; }
	FORCEINLINE bool IsMouseButton() const { return bIsMouseButton != 0; }
	FORCEINLINE bool IsAxis1D() const { return AxisType == EInputAxisType::Axis1D; }
	FORCEINLINE bool IsAxis2D() const { return AxisType == EInputAxisType::Axis2D; }
	FORCEINLINE bool IsAxis3D() const { return AxisType == EInputAxisType::Axis3D; }
	FORCEINLINE bool IsButtonAxis() const { return AxisType == EInputAxisType::Button; }	// Analog 1D axis emulating a digital button press.
	UE_DEPRECATED(4.26, "Please use IsAxis1D instead.")
	FORCEINLINE bool IsFloatAxis() const { return IsAxis1D(); }
	UE_DEPRECATED(4.26, "Please use IsAxis2D/IsAxis3D instead.")
	FORCEINLINE bool IsVectorAxis() const { return IsAxis2D() || IsAxis3D(); }
	FORCEINLINE bool IsAnalog() const { return IsAxis1D() || IsAxis2D() || IsAxis3D(); }
	FORCEINLINE bool IsDigital() const { return !IsAnalog(); }
	FORCEINLINE bool IsBindableInBlueprints() const { return bIsBindableInBlueprints != 0; }
	FORCEINLINE bool ShouldUpdateAxisWithoutSamples() const { return bShouldUpdateAxisWithoutSamples != 0; }
	FORCEINLINE bool IsBindableToActions() const { return bIsBindableToActions != 0; }
	FORCEINLINE bool IsGesture() const { return bIsGesture != 0; }
	FORCEINLINE bool IsDeprecated() const { return bIsDeprecated != 0; }
	FORCEINLINE FName GetMenuCategory() const { return MenuCategory; }
	FText GetDisplayName(const bool bLongDisplayName = true) const;
	FORCEINLINE const FKey& GetKey() const { return Key; }

	// Key pairing
	FORCEINLINE EPairedAxis GetPairedAxis() const { return PairedAxis; }
	FORCEINLINE const FKey& GetPairedAxisKey() const { return PairedAxisKey; }

private:
	friend struct EKeys;

	void CommonInit(const uint32 InKeyFlags);

	enum class EInputAxisType : uint8
	{
		None,
		Button,			// Whilst the physical input is an analog axis the FKey uses it to emulate a digital button.
		Axis1D,
		Axis2D,
		Axis3D,
	};

	FKey  Key;

	// Key pairing
	EPairedAxis PairedAxis = EPairedAxis::Unpaired;		// Paired axis identifier. Lets this key know which axis it represents on the PairedAxisKey
	FKey		PairedAxisKey;							// Paired axis reference. This is the FKey representing the final paired vector axis. Note: NOT the other key in the pairing.

	FName MenuCategory;

	uint8 bIsModifierKey : 1;
	uint8 bIsGamepadKey : 1;
	uint8 bIsTouch : 1;
	uint8 bIsMouseButton : 1;
	uint8 bIsBindableInBlueprints : 1;
	uint8 bShouldUpdateAxisWithoutSamples : 1;
	uint8 bIsBindableToActions : 1;
	uint8 bIsDeprecated : 1;
	uint8 bIsGesture : 1;
	EInputAxisType AxisType;

	TAttribute<FText> LongDisplayName;
	TAttribute<FText> ShortDisplayName;
};

UENUM(BlueprintType)
namespace ETouchIndex
{
	// The number of entries in ETouchIndex must match the number of touch keys defined in EKeys and NUM_TOUCH_KEYS above
	enum Type
	{
		Touch1,
		Touch2,
		Touch3,
		Touch4,
		Touch5,
		Touch6,
		Touch7,
		Touch8,
		Touch9,
		Touch10,
		/**
		 * This entry is special.  NUM_TOUCH_KEYS - 1, is used for the cursor so that it's represented
		 * as another finger index, but doesn't overlap with touch input indexes.
		 */
		CursorPointerIndex UMETA(Hidden),
		MAX_TOUCHES UMETA(Hidden)
	};
}

UENUM()
namespace EConsoleForGamepadLabels
{
	enum Type
	{
		None,
		XBoxOne,
		PS4
	};
}

struct INPUTCORE_API EKeys
{
	static const FKey AnyKey;

	static const FKey MouseX;
	static const FKey MouseY;
	static const FKey Mouse2D;
	static const FKey MouseScrollUp;
	static const FKey MouseScrollDown;
	static const FKey MouseWheelAxis;

	static const FKey LeftMouseButton;
	static const FKey RightMouseButton;
	static const FKey MiddleMouseButton;
	static const FKey ThumbMouseButton;
	static const FKey ThumbMouseButton2;

	static const FKey BackSpace;
	static const FKey Tab;
	static const FKey Enter;
	static const FKey Pause;

	static const FKey CapsLock;
	static const FKey Escape;
	static const FKey SpaceBar;
	static const FKey PageUp;
	static const FKey PageDown;
	static const FKey End;
	static const FKey Home;

	static const FKey Left;
	static const FKey Up;
	static const FKey Right;
	static const FKey Down;

	static const FKey Insert;
	static const FKey Delete;

	static const FKey Zero;
	static const FKey One;
	static const FKey Two;
	static const FKey Three;
	static const FKey Four;
	static const FKey Five;
	static const FKey Six;
	static const FKey Seven;
	static const FKey Eight;
	static const FKey Nine;

	static const FKey A;
	static const FKey B;
	static const FKey C;
	static const FKey D;
	static const FKey E;
	static const FKey F;
	static const FKey G;
	static const FKey H;
	static const FKey I;
	static const FKey J;
	static const FKey K;
	static const FKey L;
	static const FKey M;
	static const FKey N;
	static const FKey O;
	static const FKey P;
	static const FKey Q;
	static const FKey R;
	static const FKey S;
	static const FKey T;
	static const FKey U;
	static const FKey V;
	static const FKey W;
	static const FKey X;
	static const FKey Y;
	static const FKey Z;

	static const FKey NumPadZero;
	static const FKey NumPadOne;
	static const FKey NumPadTwo;
	static const FKey NumPadThree;
	static const FKey NumPadFour;
	static const FKey NumPadFive;
	static const FKey NumPadSix;
	static const FKey NumPadSeven;
	static const FKey NumPadEight;
	static const FKey NumPadNine;

	static const FKey Multiply;
	static const FKey Add;
	static const FKey Subtract;
	static const FKey Decimal;
	static const FKey Divide;

	static const FKey F1;
	static const FKey F2;
	static const FKey F3;
	static const FKey F4;
	static const FKey F5;
	static const FKey F6;
	static const FKey F7;
	static const FKey F8;
	static const FKey F9;
	static const FKey F10;
	static const FKey F11;
	static const FKey F12;

	static const FKey NumLock;

	static const FKey ScrollLock;

	static const FKey LeftShift;
	static const FKey RightShift;
	static const FKey LeftControl;
	static const FKey RightControl;
	static const FKey LeftAlt;
	static const FKey RightAlt;
	static const FKey LeftCommand;
	static const FKey RightCommand;

	static const FKey Semicolon;
	static const FKey Equals;
	static const FKey Comma;
	static const FKey Underscore;
	static const FKey Hyphen;
	static const FKey Period;
	static const FKey Slash;
	static const FKey Tilde;
	static const FKey LeftBracket;
	static const FKey Backslash;
	static const FKey RightBracket;
	static const FKey Apostrophe;

	static const FKey Ampersand;
	static const FKey Asterix;
	static const FKey Caret;
	static const FKey Colon;
	static const FKey Dollar;
	static const FKey Exclamation;
	static const FKey LeftParantheses;
	static const FKey RightParantheses;
	static const FKey Quote;

	static const FKey A_AccentGrave;
	static const FKey E_AccentGrave;
	static const FKey E_AccentAigu;
	static const FKey C_Cedille;
	static const FKey Section;

	// Platform Keys
	// These keys platform specific versions of keys that go by different names.
	// The delete key is a good example, on Windows Delete is the virtual key for Delete.
	// On Macs, the Delete key is the virtual key for BackSpace.
	static const FKey Platform_Delete;

	// Gamepad Keys
	static const FKey Gamepad_Left2D;
	static const FKey Gamepad_LeftX;
	static const FKey Gamepad_LeftY;
	static const FKey Gamepad_Right2D;
	static const FKey Gamepad_RightX;
	static const FKey Gamepad_RightY;
	static const FKey Gamepad_LeftTriggerAxis;
	static const FKey Gamepad_RightTriggerAxis;

	static const FKey Gamepad_LeftThumbstick;
	static const FKey Gamepad_RightThumbstick;
	static const FKey Gamepad_Special_Left;
	static const FKey Gamepad_Special_Left_X;
	static const FKey Gamepad_Special_Left_Y;
	static const FKey Gamepad_Special_Right;
	static const FKey Gamepad_FaceButton_Bottom;
	static const FKey Gamepad_FaceButton_Right;
	static const FKey Gamepad_FaceButton_Left;
	static const FKey Gamepad_FaceButton_Top;
	static const FKey Gamepad_LeftShoulder;
	static const FKey Gamepad_RightShoulder;
	static const FKey Gamepad_LeftTrigger;
	static const FKey Gamepad_RightTrigger;
	static const FKey Gamepad_DPad_Up;
	static const FKey Gamepad_DPad_Down;
	static const FKey Gamepad_DPad_Right;
	static const FKey Gamepad_DPad_Left;

	// Virtual key codes used for input axis button press/release emulation
	static const FKey Gamepad_LeftStick_Up;
	static const FKey Gamepad_LeftStick_Down;
	static const FKey Gamepad_LeftStick_Right;
	static const FKey Gamepad_LeftStick_Left;

	static const FKey Gamepad_RightStick_Up;
	static const FKey Gamepad_RightStick_Down;
	static const FKey Gamepad_RightStick_Right;
	static const FKey Gamepad_RightStick_Left;

	// static const FKey Vector axes (FVector; not float)
	static const FKey Tilt;
	static const FKey RotationRate;
	static const FKey Gravity;
	static const FKey Acceleration;

	// Gestures
	static const FKey Gesture_Pinch;
	static const FKey Gesture_Flick;
	static const FKey Gesture_Rotate;

	// PS4-specific
	UE_DEPRECATED(5.0, "This key has deprecated and will be removed. Use GamePad_Special_Left/Right instead.")
	static const FKey PS4_Special;

	// Steam Controller Specific
	static const FKey Steam_Touch_0;
	static const FKey Steam_Touch_1;
	static const FKey Steam_Touch_2;
	static const FKey Steam_Touch_3;
	static const FKey Steam_Back_Left;
	static const FKey Steam_Back_Right;

	// Xbox One global speech commands
	static const FKey Global_Menu;
	static const FKey Global_View;
	static const FKey Global_Pause;
	static const FKey Global_Play;
	static const FKey Global_Back;

	// Android-specific
	static const FKey Android_Back;
	static const FKey Android_Volume_Up;
	static const FKey Android_Volume_Down;
	static const FKey Android_Menu;

	// HTC Vive Controller
	static const FKey Vive_Left_System_Click;
	static const FKey Vive_Left_Grip_Click;
	static const FKey Vive_Left_Menu_Click;
	static const FKey Vive_Left_Trigger_Click;
	static const FKey Vive_Left_Trigger_Axis;
	static const FKey Vive_Left_Trackpad_X;
	static const FKey Vive_Left_Trackpad_Y;
	static const FKey Vive_Left_Trackpad_Click;
	static const FKey Vive_Left_Trackpad_Touch;
	static const FKey Vive_Left_Trackpad_Up;
	static const FKey Vive_Left_Trackpad_Down;
	static const FKey Vive_Left_Trackpad_Left;
	static const FKey Vive_Left_Trackpad_Right;
	static const FKey Vive_Right_System_Click;
	static const FKey Vive_Right_Grip_Click;
	static const FKey Vive_Right_Menu_Click;
	static const FKey Vive_Right_Trigger_Click;
	static const FKey Vive_Right_Trigger_Axis;
	static const FKey Vive_Right_Trackpad_X;
	static const FKey Vive_Right_Trackpad_Y;
	static const FKey Vive_Right_Trackpad_Click;
	static const FKey Vive_Right_Trackpad_Touch;
	static const FKey Vive_Right_Trackpad_Up;
	static const FKey Vive_Right_Trackpad_Down;
	static const FKey Vive_Right_Trackpad_Left;
	static const FKey Vive_Right_Trackpad_Right;

	// Microsoft Mixed Reality Motion Controller
	static const FKey MixedReality_Left_Menu_Click;
	static const FKey MixedReality_Left_Grip_Click;
	static const FKey MixedReality_Left_Trigger_Click;
	static const FKey MixedReality_Left_Trigger_Axis;
	static const FKey MixedReality_Left_Thumbstick_X;
	static const FKey MixedReality_Left_Thumbstick_Y;
	static const FKey MixedReality_Left_Thumbstick_Click;
	static const FKey MixedReality_Left_Thumbstick_Up;
	static const FKey MixedReality_Left_Thumbstick_Down;
	static const FKey MixedReality_Left_Thumbstick_Left;
	static const FKey MixedReality_Left_Thumbstick_Right;
	static const FKey MixedReality_Left_Trackpad_X;
	static const FKey MixedReality_Left_Trackpad_Y;
	static const FKey MixedReality_Left_Trackpad_Click;
	static const FKey MixedReality_Left_Trackpad_Touch;
	static const FKey MixedReality_Left_Trackpad_Up;
	static const FKey MixedReality_Left_Trackpad_Down;
	static const FKey MixedReality_Left_Trackpad_Left;
	static const FKey MixedReality_Left_Trackpad_Right;
	static const FKey MixedReality_Right_Menu_Click;
	static const FKey MixedReality_Right_Grip_Click;
	static const FKey MixedReality_Right_Trigger_Click;
	static const FKey MixedReality_Right_Trigger_Axis;
	static const FKey MixedReality_Right_Thumbstick_X;
	static const FKey MixedReality_Right_Thumbstick_Y;
	static const FKey MixedReality_Right_Thumbstick_Click;
	static const FKey MixedReality_Right_Thumbstick_Up;
	static const FKey MixedReality_Right_Thumbstick_Down;
	static const FKey MixedReality_Right_Thumbstick_Left;
	static const FKey MixedReality_Right_Thumbstick_Right;
	static const FKey MixedReality_Right_Trackpad_X;
	static const FKey MixedReality_Right_Trackpad_Y;
	static const FKey MixedReality_Right_Trackpad_Click;
	static const FKey MixedReality_Right_Trackpad_Touch;
	static const FKey MixedReality_Right_Trackpad_Up;
	static const FKey MixedReality_Right_Trackpad_Down;
	static const FKey MixedReality_Right_Trackpad_Left;
	static const FKey MixedReality_Right_Trackpad_Right;

	// Oculus Touch Controller
	static const FKey OculusTouch_Left_X_Click;
	static const FKey OculusTouch_Left_Y_Click;
	static const FKey OculusTouch_Left_X_Touch;
	static const FKey OculusTouch_Left_Y_Touch;
	static const FKey OculusTouch_Left_Menu_Click;
	static const FKey OculusTouch_Left_Grip_Click;
	static const FKey OculusTouch_Left_Grip_Axis;
	static const FKey OculusTouch_Left_Trigger_Click;
	static const FKey OculusTouch_Left_Trigger_Axis;
	static const FKey OculusTouch_Left_Trigger_Touch;
	static const FKey OculusTouch_Left_Thumbstick_X;
	static const FKey OculusTouch_Left_Thumbstick_Y;
	static const FKey OculusTouch_Left_Thumbstick_Click;
	static const FKey OculusTouch_Left_Thumbstick_Touch;
	static const FKey OculusTouch_Left_Thumbstick_Up;
	static const FKey OculusTouch_Left_Thumbstick_Down;
	static const FKey OculusTouch_Left_Thumbstick_Left;
	static const FKey OculusTouch_Left_Thumbstick_Right;
	static const FKey OculusTouch_Right_A_Click;
	static const FKey OculusTouch_Right_B_Click;
	static const FKey OculusTouch_Right_A_Touch;
	static const FKey OculusTouch_Right_B_Touch;
	static const FKey OculusTouch_Right_System_Click;
	static const FKey OculusTouch_Right_Grip_Click;
	static const FKey OculusTouch_Right_Grip_Axis;
	static const FKey OculusTouch_Right_Trigger_Click;
	static const FKey OculusTouch_Right_Trigger_Axis;
	static const FKey OculusTouch_Right_Trigger_Touch;
	static const FKey OculusTouch_Right_Thumbstick_X;
	static const FKey OculusTouch_Right_Thumbstick_Y;
	static const FKey OculusTouch_Right_Thumbstick_Click;
	static const FKey OculusTouch_Right_Thumbstick_Touch;
	static const FKey OculusTouch_Right_Thumbstick_Up;
	static const FKey OculusTouch_Right_Thumbstick_Down;
	static const FKey OculusTouch_Right_Thumbstick_Left;
	static const FKey OculusTouch_Right_Thumbstick_Right;

	// Valve Index Controller
	static const FKey ValveIndex_Left_A_Click;
	static const FKey ValveIndex_Left_B_Click;
	static const FKey ValveIndex_Left_A_Touch;
	static const FKey ValveIndex_Left_B_Touch;
	static const FKey ValveIndex_Left_System_Click;
	static const FKey ValveIndex_Left_System_Touch;
	static const FKey ValveIndex_Left_Grip_Axis;
	static const FKey ValveIndex_Left_Grip_Force;
	static const FKey ValveIndex_Left_Trigger_Click;
	static const FKey ValveIndex_Left_Trigger_Axis;
	static const FKey ValveIndex_Left_Trigger_Touch;
	static const FKey ValveIndex_Left_Thumbstick_X;
	static const FKey ValveIndex_Left_Thumbstick_Y;
	static const FKey ValveIndex_Left_Thumbstick_Click;
	static const FKey ValveIndex_Left_Thumbstick_Touch;
	static const FKey ValveIndex_Left_Thumbstick_Up;
	static const FKey ValveIndex_Left_Thumbstick_Down;
	static const FKey ValveIndex_Left_Thumbstick_Left;
	static const FKey ValveIndex_Left_Thumbstick_Right;
	static const FKey ValveIndex_Left_Trackpad_X;
	static const FKey ValveIndex_Left_Trackpad_Y;
	static const FKey ValveIndex_Left_Trackpad_Force;
	static const FKey ValveIndex_Left_Trackpad_Touch;
	static const FKey ValveIndex_Left_Trackpad_Up;
	static const FKey ValveIndex_Left_Trackpad_Down;
	static const FKey ValveIndex_Left_Trackpad_Left;
	static const FKey ValveIndex_Left_Trackpad_Right;
	static const FKey ValveIndex_Right_A_Click;
	static const FKey ValveIndex_Right_B_Click;
	static const FKey ValveIndex_Right_A_Touch;
	static const FKey ValveIndex_Right_B_Touch;
	static const FKey ValveIndex_Right_System_Click;
	static const FKey ValveIndex_Right_System_Touch;
	static const FKey ValveIndex_Right_Grip_Axis;
	static const FKey ValveIndex_Right_Grip_Force;
	static const FKey ValveIndex_Right_Trigger_Click;
	static const FKey ValveIndex_Right_Trigger_Axis;
	static const FKey ValveIndex_Right_Trigger_Touch;
	static const FKey ValveIndex_Right_Thumbstick_X;
	static const FKey ValveIndex_Right_Thumbstick_Y;
	static const FKey ValveIndex_Right_Thumbstick_Click;
	static const FKey ValveIndex_Right_Thumbstick_Touch;
	static const FKey ValveIndex_Right_Thumbstick_Up;
	static const FKey ValveIndex_Right_Thumbstick_Down;
	static const FKey ValveIndex_Right_Thumbstick_Left;
	static const FKey ValveIndex_Right_Thumbstick_Right;
	static const FKey ValveIndex_Right_Trackpad_X;
	static const FKey ValveIndex_Right_Trackpad_Y;
	static const FKey ValveIndex_Right_Trackpad_Force;
	static const FKey ValveIndex_Right_Trackpad_Touch;
	static const FKey ValveIndex_Right_Trackpad_Up;
	static const FKey ValveIndex_Right_Trackpad_Down;
	static const FKey ValveIndex_Right_Trackpad_Left;
	static const FKey ValveIndex_Right_Trackpad_Right;

	// Virtual buttons that use other buttons depending on the platform
	static const FKey Virtual_Accept;
	static const FKey Virtual_Back;

	static const FKey Invalid;

	static const int32 NUM_TOUCH_KEYS = 11;
	static const FKey TouchKeys[NUM_TOUCH_KEYS];

	// XR key names are parseable into exactly 4 tokens
	static const int32 NUM_XR_KEY_TOKENS = 4;

	static EConsoleForGamepadLabels::Type ConsoleForGamepadLabels;

	static const FName NAME_KeyboardCategory;
	static const FName NAME_GamepadCategory;
	static const FName NAME_MouseCategory;

	static void Initialize();
	static void AddKey(const FKeyDetails& KeyDetails);
	static void AddPairedKey(const FKeyDetails& PairedKeyDetails, FKey KeyX, FKey KeyY);	// Map the two provided keys to the X and Z axes of the paired key
	static void GetAllKeys(TArray<FKey>& OutKeys);
	static TSharedPtr<FKeyDetails> GetKeyDetails(const FKey Key);
	static void RemoveKeysWithCategory(const FName InCategory);

	// These exist for backwards compatibility reasons only
	static bool IsModifierKey(FKey Key) { return Key.IsModifierKey(); }
	static bool IsGamepadKey(FKey Key) { return Key.IsGamepadKey(); }
	static bool IsAxis(FKey Key) { return Key.IsAxis1D(); }
	static bool IsBindableInBlueprints(const FKey Key) { return Key.IsBindableInBlueprints(); }
	static void SetConsoleForGamepadLabels(const EConsoleForGamepadLabels::Type Console) { ConsoleForGamepadLabels = Console; }

	// Function that provides remapping for some gamepad keys in display windows
	static FText GetGamepadDisplayName(const FKey Key);

	static void AddMenuCategoryDisplayInfo(const FName CategoryName, const FText DisplayName, const FName PaletteIcon);
	static FText GetMenuCategoryDisplayName(const FName CategoryName);
	static FName GetMenuCategoryPaletteIcon(const FName CategoryName);

private:

	struct FCategoryDisplayInfo
	{
		FText DisplayName;
		FName PaletteIcon;
	};

	static TMap<FKey, TSharedPtr<FKeyDetails> > InputKeys;
	static TMap<FName, FCategoryDisplayInfo> MenuCategoryDisplayInfo;
	static bool bInitialized;

};

/** Various states of touch inputs. */
UENUM()
namespace ETouchType
{
	enum Type
	{
		Began,
		Moved,
		Stationary,
		ForceChanged,
		FirstMove,
		Ended,

		NumTypes
	};
}


struct INPUTCORE_API FInputKeyManager
{
public:
	static FInputKeyManager& Get();

	void GetCodesFromKey(const FKey Key, const uint32*& KeyCode, const uint32*& CharCode) const;

	/**
	 * Retrieves the key mapped to the specified character code.
	 * @param KeyCode	The key code to get the name for.
	 */
	FKey GetKeyFromCodes( const uint32 KeyCode, const uint32 CharCode ) const;
	void InitKeyMappings();
private:
	FInputKeyManager()
	{
		InitKeyMappings();
	}

	static TSharedPtr< FInputKeyManager > Instance;
	TMap<uint32, FKey> KeyMapVirtualToEnum;
	TMap<uint32, FKey> KeyMapCharToEnum;
};

UCLASS(abstract)
class UInputCoreTypes : public UObject
{
	GENERATED_BODY()

};
