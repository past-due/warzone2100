/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  The main interface functions to the widget library
 */

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/utf.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/gamelib/gtime.h"

#include "widget.h"

#include <utility>
#if defined(WZ_CC_MSVC)
#include "widgbase.h"		// this is generated on the pre-build event.
#endif
#include "widgint.h"

#include "form.h"
#include "label.h"
#include "button.h"
#include "editbox.h"
#include "bar.h"
#include "slider.h"
#include "tip.h"

#include <algorithm>

static	bool	bWidgetsActive = true;

/* The widget the mouse is over this update */
static WIDGET	*psMouseOverWidget = nullptr;

static WIDGET_AUDIOCALLBACK AudioCallback = nullptr;
static SWORD HilightAudioID = -1;
static SWORD ClickedAudioID = -1;
static SWORD ErrorAudioID = -1;

static WIDGET_KEY lastReleasedKey_DEPRECATED = WKEY_NONE;

static std::vector<WIDGET *> widgetDeletionQueue;

static bool debugBoundingBoxesOnly = false;


// New in-game notification system
#include <list>
#include <memory>
class WZ_Notification
{
public:
	uint32_t duration = 0; // set to 0 for "until dismissed by user"
	std::string contentTitle;
	std::string contentText;
	std::string smallIconPath;
	std::string largeIconPath;
	std::string actionButtonTitle;
	std::function<void (WZ_Notification&)> onClick;
	std::function<void (WZ_Notification&)> onDismissed;

	iV_Image* largeIcon = nullptr;
};
class WZ_Notification_Status
{
public:
	WZ_Notification_Status(uint32_t queuedTime)
	: queuedTime(queuedTime)
	{ }
	enum NotificationState
	{
		submitted,
		opening,
		shown,
		closing,
		closed
	};
	NotificationState state = NotificationState::submitted;
	uint32_t stateStartTime = 0;
	uint32_t queuedTime = 0;
};

class WZ_Notification_Trigger
{
public:
	WZ_Notification_Trigger(uint32_t timeInterval)
	: timeInterval(timeInterval)
	{ }
	static WZ_Notification_Trigger Immediate()
	{
		return WZ_Notification_Trigger(0);
	}
public:
	uint32_t timeInterval = 0;
};
class WZ_Queued_Notification
{
public:
	WZ_Queued_Notification(const WZ_Notification& notification, const WZ_Notification_Status& status, const WZ_Notification_Trigger& trigger)
	: notification(notification)
	, status(status)
	, trigger(trigger)
	{ }
	WZ_Notification notification;
	WZ_Notification_Status status;
	WZ_Notification_Trigger trigger;
};
static std::list<std::unique_ptr<WZ_Queued_Notification>> notificationQueue;
std::unique_ptr<WZ_Queued_Notification> popNextQueuedNotification()
{
	for (auto it = notificationQueue.begin(); it != notificationQueue.end(); ++it)
	{
		auto & request = *it;
		uint32_t num = std::min<uint32_t>(realTime - request->status.queuedTime, request->trigger.timeInterval);
		if (num == request->trigger.timeInterval)
		{
			std::unique_ptr<WZ_Queued_Notification> retVal(std::move(request));
			notificationQueue.erase(it);
			return retVal;
		}
	}
	return nullptr;
}

#include "lib/framework/input.h"

/** Notification initialisation structure */
struct W_NOTIFYINIT : public W_FORMINIT
{
	W_NOTIFYINIT();

	WZ_Queued_Notification* request = nullptr;
};

W_NOTIFYINIT::W_NOTIFYINIT()
: W_FORMINIT()
{ }

class W_NOTIFICATION : public W_FORM
{
public:
	W_NOTIFICATION(W_NOTIFYINIT const *init);
	// TODO: Should be able to handle drag and move notification up and down?
	void run(W_CONTEXT *psContext) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
//	void highlight(W_CONTEXT *psContext) override;
//	void highlightLost() override;
//	void display(int xOffset, int yOffset) override;
public:
	bool globalHitTest(int x, int y) { return hitTest(x, y); }
	Vector2i getDragOffset() const { return dragOffset; }
	bool isActivelyBeingDragged() const { return isInDragMode; }
	uint32_t getLastDragStartTime() const { return dragStartedTime; }
	uint32_t getLastDragEndTime() const { return dragEndedTime; }
	uint32_t getLastDragDuration() const { return dragEndedTime - dragStartedTime; }
private:
	WZ_Queued_Notification* request;
	bool DragEnabled = true;
	bool isInDragMode = false;
	Vector2i dragOffset = {0, 0};
	Vector2i dragStartMousePos = {0, 0};
	Vector2i dragOffsetEnded = {0, 0};
	uint32_t dragStartedTime = 0;
	uint32_t dragEndedTime = 0;
};

W_NOTIFICATION::W_NOTIFICATION(W_NOTIFYINIT const *init)
: W_FORM(init)
, request(init->request)
{
	// TODO:
}

static std::unique_ptr<WZ_Queued_Notification> currentNotification;
static W_NOTIFICATION* currentInGameNotification = nullptr;
static uint32_t lastNotificationClosed = 0;

// Modeled after the damped sine wave y = sin(13pi/2*x)*pow(2, 10 * (x - 1))
float ElasticEaseIn(float p)
{
	return sin(13 * M_PI_2 * p) * pow(2, 10 * (p - 1));
}

// Modeled after the damped sine wave y = sin(-13pi/2*(x + 1))*pow(2, -10x) + 1
float ElasticEaseOut(float p)
{
	return sin(-13 * M_PI_2 * (p + 1)) * pow(2, -10 * p) + 1;
}

// Modeled after the quintic y = (x - 1)^5 + 1
float QuinticEaseOut(float p)
{
	float f = (p - 1);
	return f * f * f * f * f + 1;
}

// Modeled after the cubic y = x^3
float CubicEaseIn(float p)
{
	return p * p * p;
}

float BezierBlend(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

void dismissNotification(WZ_Queued_Notification* request)
{
	// if notification is the one being displayed, animate it away by setting its state to closing
	switch (request->status.state)
	{
		case WZ_Notification_Status::NotificationState::submitted:
			// should not happen
			break;
//		case WZ_Notification_Status::NotificationState::opening:
//			request->status.state = WZ_Notification_Status::NotificationState::shown;
//			request->status.stateStartTime = realTime;
//			break;
		case WZ_Notification_Status::NotificationState::shown:
			request->status.state = WZ_Notification_Status::NotificationState::closing;
			request->status.stateStartTime = realTime;
			break;
		default:
			// do nothing
			break;
	}
}

/* Run a slider widget */
void W_NOTIFICATION::run(W_CONTEXT *psContext)
{
//	UDWORD temp1, temp2;
//	if (isInDragMode)
//	{
//		debug(LOG_3D, "In drag mode!");
//	}
	if (isInDragMode && mouseDown(MOUSE_LMB))//mouseDrag(MOUSE_LMB, &temp1, &temp2))
	{
//		int dragStartX = (int)temp1;
		int dragStartY = dragStartMousePos.y; //(int)temp2;
//		int currMouseX = mouseX();
		int currMouseY = mouseY();

		// calculate how much to respond to the drag by comparing the start to the current position
		if (dragStartY > currMouseY)
		{
			// dragging up (to close) - respond 1 to 1
			int distanceY = dragStartY - currMouseY;
			dragOffset.y = (distanceY > 0) ? -(distanceY) : 0;
		}
		else if (currMouseY > dragStartY)
		{
			// dragging down
			const int verticalLimit = 10;
			int distanceY = currMouseY - dragStartY;
			dragOffset.y = verticalLimit * (1 + log10(float(distanceY) / float(verticalLimit)));
		}
		else
		{
			dragOffset.y = 0;
		}

//		if ((dragOffset.y > 0) && (request->status.state == WZ_Notification_Status::NotificationState::closing))
//		{
//			// dragging down / open while closing
//			// set state to open
//			request->status.state = WZ_Notification_Status::NotificationState::shown;
//			request->status.stateStartTime = realTime;
//		}
//		debug(LOG_3D, "dragOffset.y: %d", dragOffset.y);
	}
	else
	{
		if (isInDragMode && !mouseDown(MOUSE_LMB))
		{
//			debug(LOG_3D, "No longer in drag mode");
			isInDragMode = false;
			dragEndedTime = realTime;
			dragOffsetEnded = dragOffset;
		}
		if (request->status.state != WZ_Notification_Status::NotificationState::closing)
		{
			// decay drag offset
			const uint32_t dragDecayDuration = GAME_TICKS_PER_SEC * 1;
			if (dragOffset.y != 0)
			{
				dragOffset.y = dragOffsetEnded.y - (int)(float(dragOffsetEnded.y) * ElasticEaseOut((float(realTime) - float(dragEndedTime)) / float(dragDecayDuration)));
			}
		}
	}
}

void W_NOTIFICATION::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
//	if (request)
//	{
//		dismissNotification(request);
//	}
	if (request->status.state == WZ_Notification_Status::NotificationState::closing)
	{
		// if clicked while closing, set state to shown
		request->status.state = WZ_Notification_Status::NotificationState::shown;
		request->status.stateStartTime = realTime;
	}

	if (DragEnabled && geometry().contains(psContext->mx, psContext->my))
	{
//		dirty = true;
		debug(LOG_3D, "Enabling drag mode");
		isInDragMode = true;
		dragStartMousePos.x = psContext->mx;
		dragStartMousePos.y = psContext->my;
		dragStartedTime = realTime;
	}
}

#define WZ_NOTIFICATION_DOWN_DRAG_DISCARD_CLICK_THRESHOLD	5

void W_NOTIFICATION::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
//	bool wasInDragMode = isInDragMode;
	debug(LOG_3D, "released");
//	isInDragMode = false;
//	dragEndedTime = realTime;
//	dragOffsetEnded = dragOffset;

	if (request)
	{
		if (!isInDragMode || dragOffset.y < WZ_NOTIFICATION_DOWN_DRAG_DISCARD_CLICK_THRESHOLD)
		{
			dismissNotification(request);
		}
	}
}

void addNotification(const WZ_Notification& notification, const WZ_Notification_Trigger& trigger)
{
	notificationQueue.push_back(std::unique_ptr<WZ_Queued_Notification>(new WZ_Queued_Notification(notification, WZ_Notification_Status(realTime), trigger)));
}


#define WZ_NOTIFICATION_PADDING	15
#define WZ_NOTIFICATION_IMAGE_SIZE	32
#define WZ_NOTIFICATION_CONTENTS_LINE_SPACING	0
#define WZ_NOTIFICATION_CONTENTS_TOP_PADDING	5

static void displayNotificationWidget(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
//	iV_DrawImage2(WzString::fromUtf8("image_fe_logo.png"), xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());

//	iV_TransBoxFill();

	// Need to do a little trick here - ensure the bounds are always positive, adjust the width and height
//	if (psWidget->y() < 0)
//	{
//		yOffset += -psWidget->y();
//	}
	int x0 = psWidget->x() + xOffset;
	int y0 = psWidget->y() + yOffset;
	int x1 = x0 + psWidget->width();
	int y1 = y0 + psWidget->height();
	if (psWidget->y() < 0)
	{
		y0 += -psWidget->y();
	}
//	debug(LOG_3D, "Draw notification widget: (%d x %d), (width: %d, height: %d) => (%dx%d),(%dx%d)", psWidget->x(), psWidget->y(), psWidget->width(), psWidget->height(), x0, y0, x1, y1);
//	int shadowPadding = 2;
//	pie_UniTransBoxFill(x0 + shadowPadding, y0 + shadowPadding, x1 + shadowPadding, y1 + shadowPadding, pal_RGBA(0, 0, 0, 70));
	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_NOTIFICATION_BOX);
	iV_Box2(x0, y0, x1, y1, WZCOL_FORM_DARK, WZCOL_FORM_DARK);

//	RenderWindowFrame(FRAME_NORMAL, psWidget->x() + xOffset, psWidget->y() + yOffset, psWidget->width(), psWidget->height());

//	// TODO: Display the title
//	iV_DrawText("", float x, float y, iV_fonts fontID)
//
//	// TODO: Display the contents
//
	// TODO: Display the icon
//	iV_DrawImage2("images/warzone2100.png", x1 - 20, y0 + 10);
	int imageLeft = x1 - WZ_NOTIFICATION_PADDING - WZ_NOTIFICATION_IMAGE_SIZE;
	int imageTop = (psWidget->y() + yOffset) + WZ_NOTIFICATION_PADDING;
	iV_Box2(imageLeft, imageTop, imageLeft + WZ_NOTIFICATION_IMAGE_SIZE, imageTop + WZ_NOTIFICATION_IMAGE_SIZE, WZCOL_RED, WZCOL_RED);
}

void removeInGameNotificationForm(WZ_Queued_Notification* request)
{
	if (!request) return;

	// right now we only support a single concurrent notification
	currentInGameNotification->deleteLater();
	currentInGameNotification = nullptr;
}

W_NOTIFICATION* getOrCreateInGameNotificationForm(WZ_Queued_Notification* request)
{
	if (!request) return nullptr;

	// right now we only support a single concurrent notification
	if (!currentInGameNotification)
	{
		W_NOTIFYINIT sFormInit;
		sFormInit.formID = 0;
		sFormInit.id = 1;
		sFormInit.request = request;
		// TODO: calculate height
//		int imgW = iV_GetImageWidth(FrontImages, IMAGE_FE_LOGO);
//		int imgH = iV_GetImageHeight(FrontImages, IMAGE_FE_LOGO);
//		int dstW = topForm->width();
//		int dstH = topForm->height();
//		if (imgW * dstH < imgH * dstW) // Want to set aspect ratio dstW/dstH = imgW/imgH.
//		{
//			dstW = imgW * dstH / imgH; // Too wide.
//		}
//		else if (imgW * dstH > imgH * dstW)
//		{
//			dstH = imgH * dstW / imgW; // Too high.
//		}
		uint16_t calculatedWidth = 500;
		uint16_t calculatedHeight = 100;
		sFormInit.x = 0; // always base of 0
		sFormInit.y = 0; // always base of 0
		sFormInit.width  = calculatedWidth;
		sFormInit.height = calculatedHeight;
		sFormInit.pDisplay = displayNotificationWidget;
		currentInGameNotification = new W_NOTIFICATION(&sFormInit);

		// Calculate dimensions for text area
		int maxTextWidth = calculatedWidth - (WZ_NOTIFICATION_PADDING * 2) - WZ_NOTIFICATION_IMAGE_SIZE - WZ_NOTIFICATION_PADDING;

		// Add title
		W_LABEL *label_title = new W_LABEL(currentInGameNotification);
		label_title->setGeometry(WZ_NOTIFICATION_PADDING, WZ_NOTIFICATION_PADDING, maxTextWidth, 12);
		label_title->setFontColour(WZCOL_TEXT_BRIGHT);
//		label->setString(WzString::fromUtf8(request->notification.contentTitle));
		int heightOfTitleLabel = label_title->setFormattedString(WzString::fromUtf8(request->notification.contentTitle), maxTextWidth, font_regular_bold, WZ_NOTIFICATION_CONTENTS_LINE_SPACING);
		label_title->setGeometry(label_title->x(), label_title->y(), maxTextWidth, heightOfTitleLabel);
		label_title->setTextAlignment(WLAB_ALIGNTOPLEFT);
//		label_title->setFont(font_regular_bold, WZCOL_TEXT_BRIGHT);
		label_title->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

		// Add contents
		W_LABEL *label_contents = new W_LABEL(currentInGameNotification);
		debug(LOG_3D, "label_title.height=%d", label_title->height());
		label_contents->setGeometry(WZ_NOTIFICATION_PADDING, WZ_NOTIFICATION_PADDING + label_title->height() + WZ_NOTIFICATION_CONTENTS_TOP_PADDING, maxTextWidth, 12);
		label_contents->setFontColour(WZCOL_TEXT_BRIGHT);
//		label_contents->setString(WzString::fromUtf8(request->notification.contentText));
		int heightOfContentsLabel = label_contents->setFormattedString(WzString::fromUtf8(request->notification.contentText), maxTextWidth, font_regular, WZ_NOTIFICATION_CONTENTS_LINE_SPACING);
		label_contents->setGeometry(label_contents->x(), label_contents->y(), maxTextWidth, heightOfContentsLabel);
		label_contents->setTextAlignment(WLAB_ALIGNTOPLEFT);
//		label_contents->setFont(font_regular, WZCOL_TEXT_BRIGHT);
		// set a custom high-testing function that ignores all mouse input / clicks
		label_contents->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

		// TODO: Add action labels
		std::string actionLabel1 = "Dismiss";
		std::string actionLabel2 = "";
		if (request->notification.duration > 0)
		{
			actionLabel1 = request->notification.actionButtonTitle;
		}
		else
		{
			actionLabel2 = request->notification.actionButtonTitle;
		}

		W_BUTINIT sButInit;
		W_BUTTON *psButton1 = nullptr;
		W_BUTTON *psButton2 = nullptr;

		// Position the buttons below the text contents area
		int buttonsTop = label_contents->y() + label_contents->height() + WZ_NOTIFICATION_PADDING;

		if (!actionLabel1.empty())
		{
			sButInit.formID = 0;
			sButInit.id = 2;
			sButInit.width = iV_GetTextWidth(actionLabel1.c_str(), font_regular_bold) + 15;
			sButInit.height = 20;
			sButInit.x = (short)(sFormInit.width - 15 - sButInit.width);
			sButInit.y = buttonsTop;
			sButInit.style |= WBUT_TXTCENTRE;

			sButInit.UserData = 0; // store disable state
	//		sButInit.pUserData = new DisplayTextOptionCache();
	//		sButInit.onDelete = [](WIDGET *psWidget) {
	//			assert(psWidget->pUserData != nullptr);
	//			delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
	//			psWidget->pUserData = nullptr;
	//		};

	//		sButInit.height = FRONTEND_BUTHEIGHT;
	//		sButInit.pDisplay = displayTextOption;
			sButInit.FontID = font_regular_bold;
			sButInit.pText = actionLabel1.c_str();
			psButton1 = new W_BUTTON(&sButInit);
			currentInGameNotification->attach(psButton1);
		}

		if (!actionLabel2.empty())
		{
			// Button 2
			sButInit.id = 3;
			sButInit.width = iV_GetTextWidth(actionLabel2.c_str(), font_regular_bold) + 15;
			sButInit.x = (short)(psButton1->x() - 15 - sButInit.width);
			sButInit.pText = actionLabel2.c_str();
			psButton2 = new W_BUTTON(&sButInit);
			currentInGameNotification->attach(psButton2);
		}

		// calculate the required height for the notification
		int bottom_labelContents = label_contents->y() + label_contents->height();
		calculatedHeight = bottom_labelContents + WZ_NOTIFICATION_PADDING;
		if (psButton1 || psButton2)
		{
			int maxButtonBottom = std::max<int>((psButton1->y() + psButton1->height()), (psButton2) ? (psButton2->y() + psButton2->height()) : 0);
			calculatedHeight = std::max<int>(calculatedHeight, maxButtonBottom + WZ_NOTIFICATION_PADDING);
		}
		// TODO: Also factor in the image, if one is present
		currentInGameNotification->setGeometry(currentInGameNotification->x(), currentInGameNotification->y(), currentInGameNotification->width(), calculatedHeight);
	}
	return currentInGameNotification;
}

#define WZ_NOTIFICATION_OPEN_DURATION (GAME_TICKS_PER_SEC*1) // Time duration for notification open/close animation
#define WZ_NOTIFICATION_CLOSE_DURATION WZ_NOTIFICATION_OPEN_DURATION
#define WZ_NOTIFICATION_TOP_PADDING 5
#define WZ_NOTIFICATION_MIN_DELAY_BETWEEN (GAME_TICKS_PER_SEC*1)

bool displayNotificationInGame(WZ_Queued_Notification* request)
{
	W_NOTIFICATION* psNotificationWidget = getOrCreateInGameNotificationForm(request);

	// center horizontally in window
	int x = std::max<int>((screenWidth - psNotificationWidget->width()) / 2, 0);
	int y = 0; // set below
//	const int startingOffscreenY = -psNotificationWidget->height();
	const int endingYPosition = WZ_NOTIFICATION_TOP_PADDING;

	// calculate positioning based on state and time
	switch (request->status.state)
	{
		case WZ_Notification_Status::NotificationState::submitted:
			// first chance to display
			request->status.state = WZ_Notification_Status::NotificationState::opening;
			request->status.stateStartTime = realTime;
			// fall-through
		case WZ_Notification_Status::NotificationState::opening:
		{
			// calculate how far we are on opening based on the stateStartTime
			uint32_t startTime = request->status.stateStartTime;
//			if (!psNotificationWidget->isActivelyBeingDragged() && psNotificationWidget->getLastDragStartTime() > startTime)
//			{
//				startTime += psNotificationWidget->getLastDragEnded();
//			}
			uint32_t endTime = startTime + WZ_NOTIFICATION_OPEN_DURATION;
			if (realTime < endTime)
			{
//				y = startingOffscreenY;
//				uint32_t
//				y += ((float(realTime) / float(endTime)) * WZ_NOTIFICATION_OPEN_DURATION)
//				y = easeChange(float(realTime) / float(endTime), startingOffscreenY, WZ_NOTIFICATION_TOP_PADDING, float(1.0));
//				y = (-psNotificationWidget->height()) + (BezierBlend((float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_OPEN_DURATION)) * (WZ_NOTIFICATION_TOP_PADDING + psNotificationWidget->height()));
				y = (-psNotificationWidget->height()) + (QuinticEaseOut((float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_OPEN_DURATION)) * (endingYPosition + psNotificationWidget->height()));
				if (!(y + psNotificationWidget->getDragOffset().y >= endingYPosition))
				{
					break;
				}
				else
				{
					// factoring in the drag, the notification is already at fully open (or past it)
					// so dropw through to immediately transition to "shown" state
				}
			}
			request->status.state = WZ_Notification_Status::NotificationState::shown;
			request->status.stateStartTime = realTime;
			// fall-through
		}
		case WZ_Notification_Status::NotificationState::shown:
		{
//			debug(LOG_ERROR, "SHOWN!");
			const auto& duration = request->notification.duration;
			if (duration > 0 && !psNotificationWidget->isActivelyBeingDragged())
			{
				if (psNotificationWidget->getDragOffset().y > 0)
				{
					// when dragging a notification more *open* (/down),
					// ensure that the notification remains displayed for at least one additional second
					// beyond the bounce-back from the drag release
					request->status.stateStartTime = std::max<uint32_t>(request->status.stateStartTime, realTime - duration + GAME_TICKS_PER_SEC);
				}
			}
			if (duration == 0 || (realTime < (request->status.stateStartTime + duration)) || psNotificationWidget->isActivelyBeingDragged())
			{
				y = endingYPosition;
				break;
			}
			request->status.state = WZ_Notification_Status::NotificationState::closing;
			request->status.stateStartTime = realTime;
			// fall-through
		}
		case WZ_Notification_Status::NotificationState::closing:
		{
			// calculate how far we are on closing based on the stateStartTime
			// factor in the current dragOffset, if any
//			if (psNotificationWidget->getDragOffset().y < 0)
//			{
//				// TODO: shorten duration based on how close to closed it is
//				// or maybe just change the easing function?
//			}
			const uint32_t &startTime = request->status.stateStartTime;
			uint32_t endTime = startTime + WZ_NOTIFICATION_CLOSE_DURATION;
			if (realTime < endTime)
			{
//				y = easeOut(float(realTime) / float(endTime), WZ_NOTIFICATION_TOP_PADDING, startingOffscreenY, float(WZ_NOTIFICATION_CLOSE_DURATION / GAME_TICKS_PER_SEC));
//				y = WZ_NOTIFICATION_TOP_PADDING - (ElasticEaseIn((float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_OPEN_DURATION)) * (WZ_NOTIFICATION_TOP_PADDING + psNotificationWidget->height()));
				float percentComplete = (float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_CLOSE_DURATION);
				if (psNotificationWidget->getDragOffset().y >= 0)
				{
					percentComplete = CubicEaseIn(percentComplete);
				}
				y = endingYPosition - (percentComplete/*CubicEaseIn((float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_OPEN_DURATION))*/ * (endingYPosition + psNotificationWidget->height()));
				if ((y + psNotificationWidget->getDragOffset().y) > -psNotificationWidget->height())
				{
					break;
				}
				else
				{
					// closed early (because of drag offset)
					// drop through and signal "closed" state
				}
			}
			request->status.state = WZ_Notification_Status::NotificationState::closed;
			request->status.stateStartTime = realTime;
			// fall-through
		}
		case WZ_Notification_Status::NotificationState::closed:
			// widget is now off-screen - destroy it
			removeInGameNotificationForm(request);
			return true; // processed notification
	}

	x += psNotificationWidget->getDragOffset().x;
	y += psNotificationWidget->getDragOffset().y;

	psNotificationWidget->move(x, y);

	/* Process any user callback functions */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	sContext.mx = mouseX();
	sContext.my = mouseY();
	psNotificationWidget->processCallbacksRecursive(&sContext);

	// Display the widgets.
	psNotificationWidget->displayRecursive(0, 0);

#if 0
	debugBoundingBoxesOnly = true;
	pie_SetRendMode(REND_ALPHA);
	psNotificationWidget->displayRecursive(0, 0);
	debugBoundingBoxesOnly = false;
#endif

	return false;
}

void runNotificationsDisplay()
{
	// at the moment, we only support displaying a single notification at a time

	if (!currentNotification)
	{
		if ((realTime - lastNotificationClosed) < WZ_NOTIFICATION_MIN_DELAY_BETWEEN)
		{
			// wait to fetch a new notification till a future cycle
			return;
		}
		// check for a new notification to display
		currentNotification = popNextQueuedNotification();
	}
	if (!currentNotification)
	{
		return;
	}

	if (displayNotificationInGame(currentNotification.get()))
	{
		// finished with this notification
		currentNotification.reset();
		lastNotificationClosed = realTime;
	}
}

// New in-game notification system


/* Initialise the widget module */
bool widgInitialise()
{
	tipInitialise();

	// TEMPORARY FOR TESTING PURPOSES:
	WZ_Notification notification;
	notification.duration = GAME_TICKS_PER_SEC * 8;
	notification.contentTitle = "Test Notification 1";
	notification.contentText = "This is a sample notification to test a very long string to see how it displays.";
	addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 30));

	notification.duration = GAME_TICKS_PER_SEC * 3;
	notification.contentTitle = "Test Notification 2";
	notification.contentText = "This is sample notification text to see how it displays.";
	addNotification(notification, WZ_Notification_Trigger::Immediate());
	notification.duration = 0;
	notification.contentTitle = "Update Available";
	notification.contentText = "A new version of Warzone 2100 (3.3.1) is available!";
	notification.actionButtonTitle = "Get Update Now";
	addNotification(notification, WZ_Notification_Trigger::Immediate());

	return true;
}


// Reset the widgets.
//
void widgReset(void)
{
	tipInitialise();
}


/* Shut down the widget module */
void widgShutDown(void)
{
}

static void deleteOldWidgets()
{
	while (!widgetDeletionQueue.empty())
	{
		WIDGET *guiltyWidget = widgetDeletionQueue.back();
		widgetDeletionQueue.pop_back();  // Do this before deleting widget, in case it calls deleteLater() on other widgets.

		ASSERT_OR_RETURN(, std::find(widgetDeletionQueue.begin(), widgetDeletionQueue.end(), guiltyWidget) == widgetDeletionQueue.end(), "Called deleteLater() twice on the same widget.");

		delete guiltyWidget;
	}
}

W_INIT::W_INIT()
	: formID(0)
	, majorID(0)
	, id(0)
	, style(0)
	, x(0), y(0)
	, width(0), height(0)
	, pDisplay(nullptr)
	, pCallback(nullptr)
	, pUserData(nullptr)
	, UserData(0)
	, calcLayout(nullptr)
	, onDelete(nullptr)
	, customHitTest(nullptr)
	, initPUserDataFunc(nullptr)
{}

WIDGET::WIDGET(W_INIT const *init, WIDGET_TYPE type)
	: id(init->id)
	, type(type)
	, style(init->style)
	, displayFunction(init->pDisplay)
	, callback(init->pCallback)
	, pUserData(init->pUserData)
	, UserData(init->UserData)
	, screenPointer(nullptr)
	, calcLayout(init->calcLayout)
	, onDelete(init->onDelete)
	, customHitTest(init->customHitTest)
	, parentWidget(nullptr)
	, dim(init->x, init->y, init->width, init->height)
	, dirty(true)
{
	/* Initialize and set the pUserData if necessary */
	if (init->initPUserDataFunc != nullptr)
	{
		assert(pUserData == nullptr); // if the initPUserDataFunc is set, pUserData should not be already set
		pUserData = init->initPUserDataFunc();
	}

	// if calclayout is not null, call it
	callCalcLayout();
}

WIDGET::WIDGET(WIDGET *parent, WIDGET_TYPE type)
	: id(0xFFFFEEEEu)
	, type(type)
	, style(0)
	, displayFunction(nullptr)
	, callback(nullptr)
	, pUserData(nullptr)
	, UserData(0)
	, screenPointer(nullptr)
	, calcLayout(nullptr)
	, onDelete(nullptr)
	, customHitTest(nullptr)
	, parentWidget(nullptr)
	, dim(0, 0, 1, 1)
	, dirty(true)
{
	parent->attach(this);
}

WIDGET::~WIDGET()
{
	if (onDelete != nullptr)
	{
		onDelete(this);	// Call the onDelete function to handle any extra logic
	}

	setScreenPointer(nullptr);  // Clear any pointers to us directly from screenPointer.
	tipStop(this);  // Stop showing tooltip, if we are.

	if (parentWidget != nullptr)
	{
		parentWidget->detach(this);
	}
	for (unsigned n = 0; n < childWidgets.size(); ++n)
	{
		childWidgets[n]->parentWidget = nullptr;  // Detach in advance, slightly faster than detach(), and doesn't change our list.
		delete childWidgets[n];
	}
}

void WIDGET::deleteLater()
{
	widgetDeletionQueue.push_back(this);
}

void WIDGET::setGeometry(WzRect const &r)
{
	if (dim == r)
	{
		return;  // Nothing to do.
	}
	dim = r;
	geometryChanged();
	dirty = true;
}

void WIDGET::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// Default implementation of screenSizeDidChange calls its own calcLayout callback function (if present)
	callCalcLayout();

	// Then propagates the event to all children
	for (Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;
		psCurr->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	}
}

void WIDGET::setCalcLayout(const WIDGET_CALCLAYOUT_FUNC& calcLayoutFunc)
{
	calcLayout = calcLayoutFunc;
	callCalcLayout();
}

void WIDGET::callCalcLayout()
{
	if (calcLayout)
	{
		calcLayout(this, screenWidth, screenHeight, screenWidth, screenHeight);
	}
#ifdef DEBUG
//	// FOR DEBUGGING:
//	// To help track down WIDGETs missing a calc layout function
//	// (because something isn't properly re-adjusting when live window resizing occurs)
//	// uncomment the else branch below.
//	else
//	{
//		debug(LOG_ERROR, "Object is missing calc layout function");
//	}
#endif
}

void WIDGET::setOnDelete(const WIDGET_ONDELETE_FUNC& onDeleteFunc)
{
	onDelete = onDeleteFunc;
}

void WIDGET::setCustomHitTest(const WIDGET_HITTEST_FUNC& newCustomHitTestFunc)
{
	customHitTest = newCustomHitTestFunc;
}

void WIDGET::attach(WIDGET *widget)
{
	ASSERT_OR_RETURN(, widget != nullptr && widget->parentWidget == nullptr, "Bad attach.");
	widget->parentWidget = this;
	widget->setScreenPointer(screenPointer);
	childWidgets.push_back(widget);
}

void WIDGET::detach(WIDGET *widget)
{
	ASSERT_OR_RETURN(, widget != nullptr && widget->parentWidget != nullptr, "Bad detach.");

	widget->parentWidget = nullptr;
	widget->setScreenPointer(nullptr);
	childWidgets.erase(std::find(childWidgets.begin(), childWidgets.end(), widget));

	widgetLost(widget);
}

void WIDGET::setScreenPointer(W_SCREEN *screen)
{
	if (screenPointer == screen)
	{
		return;
	}

	if (psMouseOverWidget == this)
	{
		psMouseOverWidget = nullptr;
	}
	if (screenPointer != nullptr && screenPointer->psFocus == this)
	{
		screenPointer->psFocus = nullptr;
	}
	if (screenPointer != nullptr && screenPointer->lastHighlight == this)
	{
		screenPointer->lastHighlight = nullptr;
	}

	screenPointer = screen;
	for (Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		(*i)->setScreenPointer(screen);
	}
}

void WIDGET::widgetLost(WIDGET *widget)
{
	if (parentWidget != nullptr)
	{
		parentWidget->widgetLost(widget);  // We don't care about the lost widget, maybe the parent does. (Special code for W_TABFORM.)
	}
}

W_SCREEN::W_SCREEN()
	: psFocus(nullptr)
	, lastHighlight(nullptr)
	, TipFontID(font_regular)
{
	W_FORMINIT sInit;
	sInit.id = 0;
	sInit.style = WFORM_PLAIN | WFORM_INVISIBLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;

	psForm = new W_FORM(&sInit);
	psForm->screenPointer = this;
}

W_SCREEN::~W_SCREEN()
{
	delete psForm;
}

void W_SCREEN::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// resize the top-level form
	psForm->setGeometry(0, 0, screenWidth - 1, screenHeight - 1);

	// inform the top-level form of the event
	psForm->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

/* Check whether an ID has been used on a form */
static bool widgCheckIDForm(WIDGET *psForm, UDWORD id)
{
	if (psForm->id == id)
	{
		return true;
	}
	for (WIDGET::Children::const_iterator i = psForm->children().begin(); i != psForm->children().end(); ++i)
	{
		if (widgCheckIDForm(*i, id))
		{
			return true;
		}
	}
	return false;
}

static bool widgAddWidget(W_SCREEN *psScreen, W_INIT const *psInit, WIDGET *widget)
{
	ASSERT_OR_RETURN(false, widget != nullptr, "Invalid widget");
	ASSERT_OR_RETURN(false, psScreen != nullptr, "Invalid screen pointer");
	ASSERT_OR_RETURN(false, !widgCheckIDForm(psScreen->psForm, psInit->id), "ID number has already been used (%d)", psInit->id);
	// Find the form to add the widget to.
	W_FORM *psParent;
	if (psInit->formID == 0)
	{
		/* Add to the base form */
		psParent = psScreen->psForm;
	}
	else
	{
		psParent = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
	}
	ASSERT_OR_RETURN(false, psParent != nullptr && psParent->type == WIDG_FORM, "Could not find parent form from formID");

	psParent->attach(widget);
	return true;
}

/* Add a form to the widget screen */
W_FORM *widgAddForm(W_SCREEN *psScreen, const W_FORMINIT *psInit)
{
	W_FORM *psForm;
	if (psInit->style & WFORM_CLICKABLE)
	{
		psForm = new W_CLICKFORM(psInit);
	}
	else
	{
		psForm = new W_FORM(psInit);
	}

	return widgAddWidget(psScreen, psInit, psForm) ? psForm : nullptr;
}

/* Add a label to the widget screen */
W_LABEL *widgAddLabel(W_SCREEN *psScreen, const W_LABINIT *psInit)
{
	W_LABEL *psLabel = new W_LABEL(psInit);
	return widgAddWidget(psScreen, psInit, psLabel) ? psLabel : nullptr;
}

/* Add a button to the widget screen */
W_BUTTON *widgAddButton(W_SCREEN *psScreen, const W_BUTINIT *psInit)
{
	W_BUTTON *psButton = new W_BUTTON(psInit);
	return widgAddWidget(psScreen, psInit, psButton) ? psButton : nullptr;
}

/* Add an edit box to the widget screen */
W_EDITBOX *widgAddEditBox(W_SCREEN *psScreen, const W_EDBINIT *psInit)
{
	W_EDITBOX *psEdBox = new W_EDITBOX(psInit);
	return widgAddWidget(psScreen, psInit, psEdBox) ? psEdBox : nullptr;
}

/* Add a bar graph to the widget screen */
W_BARGRAPH *widgAddBarGraph(W_SCREEN *psScreen, const W_BARINIT *psInit)
{
	W_BARGRAPH *psBarGraph = new W_BARGRAPH(psInit);
	return widgAddWidget(psScreen, psInit, psBarGraph) ? psBarGraph : nullptr;
}

/* Add a slider to a form */
W_SLIDER *widgAddSlider(W_SCREEN *psScreen, const W_SLDINIT *psInit)
{
	W_SLIDER *psSlider = new W_SLIDER(psInit);
	return widgAddWidget(psScreen, psInit, psSlider) ? psSlider : nullptr;
}

/* Delete a widget from the screen */
void widgDelete(W_SCREEN *psScreen, UDWORD id)
{
	delete widgGetFromID(psScreen, id);
}

/* Find a widget on a form from its id number */
static WIDGET *widgFormGetFromID(WIDGET *widget, UDWORD id)
{
	if (widget->id == id)
	{
		return widget;
	}
	WIDGET::Children const &c = widget->children();
	for (WIDGET::Children::const_iterator i = c.begin(); i != c.end(); ++i)
	{
		WIDGET *w = widgFormGetFromID(*i, id);
		if (w != nullptr)
		{
			return w;
		}
	}
	return nullptr;
}

/* Find a widget in a screen from its ID number */
WIDGET *widgGetFromID(W_SCREEN *psScreen, UDWORD id)
{
	return widgFormGetFromID(psScreen->psForm, id);
}

void widgHide(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget from id.");

	psWidget->hide();
}

void widgReveal(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget from id.");

	psWidget->show();
}


/* Get the current position of a widget */
void widgGetPos(W_SCREEN *psScreen, UDWORD id, SWORD *pX, SWORD *pY)
{
	WIDGET	*psWidget;

	/* Find the widget */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget != nullptr)
	{
		*pX = psWidget->x();
		*pY = psWidget->y();
	}
	else
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find widget by ID");
		*pX = 0;
		*pY = 0;
	}
}

/* Return the ID of the widget the mouse was over this frame */
UDWORD widgGetMouseOver(W_SCREEN *psScreen)
{
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if psMouseOverWidget needs to stop being a static and moves into
	   the screen structure */
	(void)psScreen;

	if (psMouseOverWidget == nullptr)
	{
		return 0;
	}

	return psMouseOverWidget->id;
}


/* Return the user data for a widget */
void *widgGetUserData(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->pUserData;
	}

	return nullptr;
}


/* Return the user data for a widget */
UDWORD widgGetUserData2(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->UserData;
	}

	return 0;
}


/* Set user data for a widget */
void widgSetUserData(W_SCREEN *psScreen, UDWORD id, void *UserData)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->pUserData = UserData;
	}
}

/* Set user data for a widget */
void widgSetUserData2(W_SCREEN *psScreen, UDWORD id, UDWORD UserData)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->UserData = UserData;
	}
}

void WIDGET::setTip(std::string)
{
	ASSERT(false, "Can't set widget type %u's tip.", type);
}

/* Set tip string for a widget */
void widgSetTip(W_SCREEN *psScreen, UDWORD id, std::string pTip)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);

	if (!psWidget)
	{
		return;
	}

	psWidget->setTip(std::move(pTip));
}

/* Return which key was used to press the last returned widget */
UDWORD widgGetButtonKey_DEPRECATED(W_SCREEN *psScreen)
{
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if released needs to stop being a static and moves into
	   the screen structure */
	(void)psScreen;

	return lastReleasedKey_DEPRECATED;
}

unsigned WIDGET::getState()
{
	ASSERT(false, "Can't get widget type %u's state.", type);
	return 0;
}

/* Get a button or clickable form's state */
UDWORD widgGetButtonState(W_SCREEN *psScreen, UDWORD id)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(0, psWidget, "Couldn't find widget by ID %u", id);

	return psWidget->getState();
}

void WIDGET::setFlash(bool)
{
	ASSERT(false, "Can't set widget type %u's flash.", type);
}

void widgSetButtonFlash(W_SCREEN *psScreen, UDWORD id)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(true);
}

void widgClearButtonFlash(W_SCREEN *psScreen, UDWORD id)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(false);
}


void WIDGET::setState(unsigned)
{
	ASSERT(false, "Can't set widget type %u's state.", type);
}

/* Set a button or clickable form's state */
void widgSetButtonState(W_SCREEN *psScreen, UDWORD id, UDWORD state)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setState(state);
}

WzString WIDGET::getString() const
{
	ASSERT(false, "Can't get widget type %u's string.", type);
	return WzString();
}

/* Return a pointer to a buffer containing the current string of a widget.
 * NOTE: The string must be copied out of the buffer
 */
const char *widgGetString(W_SCREEN *psScreen, UDWORD id)
{
	const WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN("", psWidget, "Couldn't find widget by ID %u", id);

	static WzString ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString();
	return ret.toUtf8().c_str();
}

const WzString& widgGetWzString(W_SCREEN *psScreen, UDWORD id)
{
	static WzString emptyString = WzString();
	const WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(emptyString, psWidget, "Couldn't find widget by ID %u", id);

	static WzString ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString();
	return ret;
}

void WIDGET::setString(WzString)
{
	ASSERT(false, "Can't set widget type %u's string.", type);
}

/* Set the text in a widget */
void widgSetString(W_SCREEN *psScreen, UDWORD id, const char *pText)
{
	/* Get the widget */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	if (psWidget->type == WIDG_EDITBOX && psScreen->psFocus == psWidget)
	{
		psScreen->setFocus(nullptr);
	}

	psWidget->setString(WzString::fromUtf8(pText));
}

void WIDGET::processCallbacksRecursive(W_CONTEXT *psContext)
{
	/* Go through all the widgets on the form */
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		/* Call the callback */
		if (psCurr->callback)
		{
			psCurr->callback(psCurr, psContext);
		}

		/* and then recurse */
		W_CONTEXT sFormContext;
		sFormContext.mx = psContext->mx - psCurr->x();
		sFormContext.my = psContext->my - psCurr->y();
		sFormContext.xOffset = psContext->xOffset + psCurr->x();
		sFormContext.yOffset = psContext->yOffset + psCurr->y();
		psCurr->processCallbacksRecursive(&sFormContext);
	}
}

/* Process all the widgets on a form.
 * mx and my are the coords of the mouse relative to the form origin.
 */
void WIDGET::runRecursive(W_CONTEXT *psContext)
{
	/* Process the form's widgets */
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		/* Skip any hidden widgets */
		if (!psCurr->visible())
		{
			continue;
		}

		/* Found a sub form, so set up the context */
		W_CONTEXT sFormContext;
		sFormContext.mx = psContext->mx - psCurr->x();
		sFormContext.my = psContext->my - psCurr->y();
		sFormContext.xOffset = psContext->xOffset + psCurr->x();
		sFormContext.yOffset = psContext->yOffset + psCurr->y();

		/* Process it */
		psCurr->runRecursive(&sFormContext);

		/* Run the widget */
		psCurr->run(psContext);
	}
}

bool WIDGET::hitTest(int x, int y)
{
	// default hit-testing bounding rect (based on the widget's x, y, width, height)
	bool hitTestResult = dim.contains(x, y);

	if(customHitTest)
	{
		// if the default bounding-rect hit-test succeeded, use the custom hit-testing func
		hitTestResult = hitTestResult && customHitTest(this, x, y);
	}

	return hitTestResult;
}

bool WIDGET::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	bool didProcessClick = false;

	W_CONTEXT shiftedContext;
	shiftedContext.mx = psContext->mx - x();
	shiftedContext.my = psContext->my - y();
	shiftedContext.xOffset = psContext->xOffset + x();
	shiftedContext.yOffset = psContext->yOffset + y();

	// Process subwidgets.
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		if (!psCurr->visible() || !psCurr->hitTest(shiftedContext.mx, shiftedContext.my))
		{
			continue;  // Skip any hidden widgets, or widgets the click missed.
		}

		// Process it (recursively).
		didProcessClick = psCurr->processClickRecursive(&shiftedContext, key, wasPressed) || didProcessClick;
	}

	if (psMouseOverWidget == nullptr)
	{
		psMouseOverWidget = this;  // Mark that the mouse is over a widget (if we haven't already).
	}
	if (screenPointer && screenPointer->lastHighlight != this && psMouseOverWidget == this)
	{
		if (screenPointer->lastHighlight != nullptr)
		{
			screenPointer->lastHighlight->highlightLost();
		}
		screenPointer->lastHighlight = this;  // Mark that the mouse is over a widget (if we haven't already).
		highlight(psContext);
	}

	if (key == WKEY_NONE)
	{
		return false;  // Just checking mouse position, not a click.
	}

	if (psMouseOverWidget == this)
	{
		if (wasPressed)
		{
			clicked(psContext, key);
		}
		else
		{
			released(psContext, key);
		}
		didProcessClick = true;
	}

	return didProcessClick;
}


/* Execute a set of widgets for one cycle.
 * Returns a list of activated widgets.
 */
WidgetTriggers const &widgRunScreen(W_SCREEN *psScreen)
{
	psScreen->retWidgets.clear();

	/* Initialise the context */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	psMouseOverWidget = nullptr;

//	// Tell the current notification, if present, which screen is its (current) "parent"
//	if (currentInGameNotification)
//	{
//		currentInGameNotification->screenPointer = psScreen;
//	}

	// Note which keys have been pressed
	lastReleasedKey_DEPRECATED = WKEY_NONE;
	if (getWidgetsStatus())
	{
		MousePresses const &clicks = inputGetClicks();
		for (MousePresses::const_iterator c = clicks.begin(); c != clicks.end(); ++c)
		{
			WIDGET_KEY wkey;
			switch (c->key)
			{
			case MOUSE_LMB: wkey = WKEY_PRIMARY; break;
			case MOUSE_RMB: wkey = WKEY_SECONDARY; break;
			default: continue;  // Who cares about other mouse buttons?
			}
			bool pressed;
			switch (c->action)
			{
			case MousePress::Press: pressed = true; break;
			case MousePress::Release: pressed = false; break;
			default: continue;
			}
			sContext.mx = c->pos.x;
			sContext.my = c->pos.y;
			if (currentInGameNotification && currentInGameNotification->globalHitTest(sContext.mx, sContext.my))
			{
				// Forward first to in-game notifications, which are above everything else
				currentInGameNotification->processClickRecursive(&sContext, wkey, pressed);
			}
			psScreen->psForm->processClickRecursive(&sContext, wkey, pressed);

			lastReleasedKey_DEPRECATED = wkey;
		}
	}

	sContext.mx = mouseX();
	sContext.my = mouseY();
	if (currentInGameNotification && currentInGameNotification->globalHitTest(sContext.mx, sContext.my))
	{
		// Forward first to in-game notifications, which are above everything else
		currentInGameNotification->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.
	}
	psScreen->psForm->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.

	/* Process the screen's widgets */
	if (currentInGameNotification)
	{
		// Forward first to in-game notifications, which are above everything else
		currentInGameNotification->runRecursive(&sContext);
		/* Run the widget */
		currentInGameNotification->run(&sContext);
	}
	psScreen->psForm->runRecursive(&sContext);

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being run.

	/* Return the ID of a pressed button or finished edit box if any */
	return psScreen->retWidgets;
}

/* Set the id number for widgRunScreen to return */
void W_SCREEN::setReturn(WIDGET *psWidget)
{
	WidgetTrigger trigger;
	trigger.widget = psWidget;
	retWidgets.push_back(trigger);
}

void WIDGET::displayRecursive(int xOffset, int yOffset)
{
	if (debugBoundingBoxesOnly)
	{
		// Display bounding boxes.
		PIELIGHT col;
		col.byte.r = 128 + iSinSR(realTime, 2000, 127); col.byte.g = 128 + iSinSR(realTime + 667, 2000, 127); col.byte.b = 128 + iSinSR(realTime + 1333, 2000, 127); col.byte.a = 128;
		iV_Box(xOffset + x(), yOffset + y(), xOffset + x() + width() - 1, yOffset + y() + height() - 1, col);
	}
	else if (displayFunction)
	{
		displayFunction(this, xOffset, yOffset);
	}
	else
	{
		// Display widget.
		display(xOffset, yOffset);
	}

	if (type == WIDG_FORM && ((W_FORM *)this)->disableChildren)
	{
		return;
	}

	// Update the offset from the current widget's position.
	xOffset += x();
	yOffset += y();

	// If this is a clickable form, the widgets on it have to move when it's down.
	if (type == WIDG_FORM && (((W_FORM *)this)->style & WFORM_NOCLICKMOVE) == 0)
	{
		if ((((W_FORM *)this)->style & WFORM_CLICKABLE) != 0 &&
		    (((W_CLICKFORM *)this)->state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0)
		{
			++xOffset;
			++yOffset;
		}
	}

	// Display the widgets on this widget.
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		// Skip any hidden widgets.
		if (!psCurr->visible())
		{
			continue;
		}

		psCurr->displayRecursive(xOffset, yOffset);
	}
}

/* Display the screen's widgets in their current state
 * (Call after calling widgRunScreen, this allows the input
 *  processing to be separated from the display of the widgets).
 */
void widgDisplayScreen(W_SCREEN *psScreen)
{
	// To toggle debug bounding boxes: Press: Left Shift   --  --  --------------
	//                                        Left Ctrl  ------------  --  --  ----
	static const int debugSequence[] = { -1, 0, 1, 3, 1, 3, 1, 3, 2, 3, 2, 3, 2, 3, 1, 0, -1};
	static int const *debugLoc = debugSequence;
	static bool debugBoundingBoxes = false;
	int debugCode = keyDown(KEY_LCTRL) + 2 * keyDown(KEY_LSHIFT);
	debugLoc = debugLoc[1] == -1 ? debugSequence : debugLoc[0] == debugCode ? debugLoc : debugLoc[1] == debugCode ? debugLoc + 1 : debugSequence;
	debugBoundingBoxes = debugBoundingBoxes ^ (debugLoc[1] == -1);

	/* Process any user callback functions */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	sContext.mx = mouseX();
	sContext.my = mouseY();
	psScreen->psForm->processCallbacksRecursive(&sContext);

	// Display the widgets.
	psScreen->psForm->displayRecursive(0, 0);

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being displayed.

	/* Display the tool tip if there is one */
	tipDisplay();

	if (debugBoundingBoxes)
	{
		debugBoundingBoxesOnly = true;
		pie_SetRendMode(REND_ALPHA);
		psScreen->psForm->displayRecursive(0, 0);
		debugBoundingBoxesOnly = false;
	}

	// Always display notifications on-top (i.e. draw them last)
	runNotificationsDisplay();
}

void W_SCREEN::setFocus(WIDGET *widget)
{
	if (psFocus != nullptr)
	{
		psFocus->focusLost();
	}
	psFocus = widget;
}

void WidgSetAudio(WIDGET_AUDIOCALLBACK Callback, SWORD HilightID, SWORD ClickedID, SWORD ErrorID)
{
	AudioCallback = Callback;
	HilightAudioID = HilightID;
	ClickedAudioID = ClickedID;
	ErrorAudioID = ErrorID;
}

WIDGET_AUDIOCALLBACK WidgGetAudioCallback(void)
{
	return AudioCallback;
}

SWORD WidgGetHilightAudioID(void)
{
	return HilightAudioID;
}

SWORD WidgGetClickedAudioID(void)
{
	return ClickedAudioID;
}

SWORD WidgGetErrorAudioID(void)
{
	return ErrorAudioID;
}

void setWidgetsStatus(bool var)
{
	bWidgetsActive = var;
}

bool getWidgetsStatus()
{
	return bWidgetsActive;
}
