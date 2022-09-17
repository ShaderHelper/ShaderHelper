// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/ICursor.h"
#include "Framework/Application/IInputProcessor.h"

class FSlateApplication;
class FSlateUser;
struct FInputEvent;
struct FAnalogInputEvent;
struct FKeyEvent;
struct FPointerEvent;

namespace AnalogCursorMode
{
	enum Type
	{
		Accelerated,
		Direct,
	};
}

enum class EAnalogStick : uint8
{
	Left,
	Right,
	Max,
};

/**
 * A class that simulates a cursor driven by an analog stick.
 */
class SLATE_API FAnalogCursor : public IInputProcessor, public TSharedFromThis<FAnalogCursor>
{
public:
	FAnalogCursor();

	/** Dtor */
	virtual ~FAnalogCursor()
	{}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("AnalogCursor"); }


	virtual int32 GetOwnerUserIndex() const { return 0; };

	void SetAcceleration(float NewAcceleration);
	void SetMaxSpeed(float NewMaxSpeed);
	void SetStickySlowdown(float NewStickySlowdown);
	void SetDeadZone(float NewDeadZone);
	void SetMode(AnalogCursorMode::Type NewMode);

protected:

	virtual bool IsRelevantInput(const FInputEvent& InputEvent) const;
	virtual bool IsRelevantInput(const FKeyEvent& KeyEvent) const;
	virtual bool IsRelevantInput(const FAnalogInputEvent& AnalogInputEvent) const;
	virtual bool IsRelevantInput(const FPointerEvent& MouseEvent) const;

	/** Getter */
	FORCEINLINE const FVector2D& GetAnalogValues( EAnalogStick Stick = EAnalogStick::Left ) const
	{
		return AnalogValues[ static_cast< uint8 >( Stick ) ];
	}

	/** Set the cached analog stick declinations to 0 */
	void ClearAnalogValues();

	/** Handles updating the cursor position and processing a Mouse Move Event */
	UE_DEPRECATED(4.24, "FAnalogCursor now updates cursor position based on user, not the hardware cursor specifically.")
	virtual void UpdateCursorPosition(FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor, const FVector2D& NewPosition, bool bForce = false);

	virtual void UpdateCursorPosition(FSlateApplication& SlateApp, TSharedRef<FSlateUser> SlateUser, const FVector2D& NewPosition, bool bForce = false);

	/** Current speed of the cursor */
	FVector2D CurrentSpeed;

	/** Current sub-pixel offset */
	FVector2D CurrentOffset;

	/** Current settings */
	float Acceleration;
	float MaxSpeed;
	float StickySlowdown;
	float DeadZone;
	AnalogCursorMode::Type Mode;

private:

	FORCEINLINE FVector2D& GetAnalogValue( EAnalogStick Stick )
	{
		return AnalogValues[ static_cast< uint8 >( Stick ) ];
	}

	/** Input from the gamepad */
	FVector2D AnalogValues[ static_cast<uint8>( EAnalogStick::Max ) ];
};

