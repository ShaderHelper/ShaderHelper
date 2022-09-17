// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Attribute.h"
#include "Logging/LogCategory.h"
#include "Internationalization/Text.h"

struct FSlateBrush;
class IAsyncTaskNotificationImpl;

/**
 * Configuration data for initializing an asynchronous task notification.
 */
struct FAsyncTaskNotificationConfig
{
	/** The title text displayed in the notification */
	FText TitleText;

	/** The progress text displayed in the notification (if any) */
	FText ProgressText;

	/** The fade in duration of the notification */
	float FadeInDuration = 0.5f;

	/** The fade out duration of the notification */
	float FadeOutDuration = 2.0f;

	/** The duration before a fadeout for the notification */
	float ExpireDuration = 1.0f;

	/** Should this notification be "headless"? (ie, not display any UI) */
	bool bIsHeadless = false;

	/** Can this task be canceled? Will show a cancel button for in-progress tasks (attribute queried from the game thread) */
	TAttribute<bool> bCanCancel = false;

	/** Keep this notification open on success? Will show a close button (attribute queried from the game thread) */
	TAttribute<bool> bKeepOpenOnSuccess = false;

	/** Keep this notification open on failure? Will show an close button (attribute queried from the game thread) */
	TAttribute<bool> bKeepOpenOnFailure = false;

	/** The icon image to display next to the text, or null to use the default icon */
	const FSlateBrush* Icon = nullptr;

	/** Category this task should log its notifications under, or null to skip logging */
	const FLogCategoryBase* LogCategory = nullptr;
};

enum class EAsyncTaskNotificationState : uint8
{
	None = 0,
	Pending,
	Success,
	Failure,
	Prompt,
};

enum class EAsyncTaskNotificationPromptAction : uint8
{
	/** No action taken for the ongoing task. */
	None = 0,
	/** Continue ongoing task after a prompt. */
	Continue,
	/** Cancel ongoing task. */
	Cancel,
	/** No action can be taken, task is unattended. */
	Unattended
};

/**
 * Async Notification State data
 */
struct FAsyncNotificationStateData
{
	FAsyncNotificationStateData()
		: State(EAsyncTaskNotificationState::Pending)
	{}

	FAsyncNotificationStateData(const FText& InTitleText, const FText& InProgressText, EAsyncTaskNotificationState InState)
		: State(InState)
		, TitleText(InTitleText)
		, ProgressText(InProgressText)
	{}

	/** The notification state. */
	EAsyncTaskNotificationState State;

	/** The title text displayed in the notification */
	FText TitleText;

	/** The progress text displayed in the notification (if any) */
	FText ProgressText;

	/** The prompt text displayed in the notification, follow the progress text in console notification, used on button for UI notification (if any) */
	FText PromptText;

	/** Text to display for the hyperlink message. */
	FText HyperlinkText;

	/** Hyperlink callback. If not bound the hyperlink text won't be displayed. */
	FSimpleDelegate Hyperlink;
};

/**
 * Provides notifications for an on-going asynchronous task.
 */
class CORE_API FAsyncTaskNotification
{
public:
	/**
	 * Create an asynchronous task notification.
	 */
	FAsyncTaskNotification(const FAsyncTaskNotificationConfig& InConfig);

	/**
	 * Destroy the asynchronous task notification.
	 */
	~FAsyncTaskNotification();

	/**
	 * Non-copyable.
	 */
	FAsyncTaskNotification(const FAsyncTaskNotification&) = delete;
	FAsyncTaskNotification& operator=(const FAsyncTaskNotification&) = delete;

	/**
	 * Movable.
	 */
	FAsyncTaskNotification(FAsyncTaskNotification&& InOther);
	FAsyncTaskNotification& operator=(FAsyncTaskNotification&& InOther);

	/**
	 * Set the title text of this notification.
	 */
	void SetTitleText(const FText& InTitleText, const bool bClearProgressText = true);

	/**
	 * Set the progress text of this notification.
	 */
	void SetProgressText(const FText& InProgressText);

	/**
	 * Set the prompt text of this notification, if needed.
	 */
	void SetPromptText(const FText& InPromptText);

	/**
	 * Set the Hyperlink for this notification.
	 */
	void SetHyperlink(const FSimpleDelegate& InHyperlink, const FText& InHyperlinkText = FText());

	/**
	 * Set the task as complete.
	 */
	void SetComplete(const bool bSuccess = true);

	/**
	 * Update the text and set the task as complete.
	 */
	void SetComplete(const FText& InTitleText, const FText& InProgressText, const bool bSuccess = true);

	/**
	 * Set the notification state. Provide finer control than SetComplete by setting every field of the state.
	 */
	void SetNotificationState(const FAsyncNotificationStateData& InState);

	/**
	 * Set whether this task be canceled.
	 */
	void SetCanCancel(const TAttribute<bool>& InCanCancel);

	/**
	 * Set whether to keep this notification open on success.
	*/
	void SetKeepOpenOnSuccess(const TAttribute<bool>& InKeepOpenOnSuccess);

	/**
	 * Set whether to keep this notification open on failure.
	 */
	void SetKeepOpenOnFailure(const TAttribute<bool>& InKeepOpenOnFailure);

	/**
	 * Return the notification prompt action.
	 * The action resets to `None` when the notification state changes.
	 */
	EAsyncTaskNotificationPromptAction GetPromptAction() const;

private:
	/** Pointer to the real notification implementation */
	IAsyncTaskNotificationImpl* NotificationImpl = nullptr;
};
