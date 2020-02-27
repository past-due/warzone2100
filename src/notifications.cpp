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
/**
 * @file init.c
 *
 * Game initialisation routines.
 *
 */
#include "lib/framework/frame.h"
#include "notifications.h"
#include "lib/gamelib/gtime.h"
#include "lib/widget/form.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pietypes.h"

// New in-game notification system

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

class WZ_Queued_Notification
{
public:
	typedef std::function<void (WZ_Queued_Notification *)> ProcessedRequestCallback;
public:
	WZ_Queued_Notification(const WZ_Notification& notification, const WZ_Notification_Status& status, const WZ_Notification_Trigger& trigger)
	: notification(notification)
	, status(status)
	, trigger(trigger)
	{ }
	void setProcessedNotificationRequestCallback(const ProcessedRequestCallback& func) { onProcessedNotificationRequestFunc = func; }
	void processedRequest() { if(onProcessedNotificationRequestFunc) { onProcessedNotificationRequestFunc(this); } }
public:
	WZ_Notification notification;
	WZ_Notification_Status status;
	WZ_Notification_Trigger trigger;
private:
	ProcessedRequestCallback onProcessedNotificationRequestFunc;
};

// TEMPORARY
void finishedProcessingNotificationRequest(WZ_Queued_Notification* request);

//

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
	bool calculateNotificationWidgetPos();
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


struct WzCheckboxButton : public W_BUTTON
{
	WzCheckboxButton(WIDGET *parent) : W_BUTTON(parent) {}

	void display(int xOffset, int yOffset);

private:
	Image imNormal;
	Image imDown;
	unsigned doHighlight;
};
//
//Image mpwidgetGetFrontHighlightImage(Image image)
//{
//	if (image.isNull())
//	{
//		return Image();
//	}
//	switch (image.width())
//	{
//	case 30: return Image(FrontImages, IMAGE_HI34);
//	case 60: return Image(FrontImages, IMAGE_HI64);
//	case 19: return Image(FrontImages, IMAGE_HI23);
//	case 27: return Image(FrontImages, IMAGE_HI31);
//	case 35: return Image(FrontImages, IMAGE_HI39);
//	case 37: return Image(FrontImages, IMAGE_HI41);
//	case 56: return Image(FrontImages, IMAGE_HI56);
//	}
//	return Image();
//}

void WzCheckboxButton::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	Image hiToUse(nullptr, 0);

	// evaluate auto-frame
	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;

	// evaluate auto-frame
	if (doHighlight == 1 && highlight)
	{
//		hiToUse = mpwidgetGetFrontHighlightImage(imNormal);
	}

	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool grey = (getState() & WBUT_DISABLE) != 0;

	Image toDraw[3];
	int numToDraw = 0;

	// now display
	toDraw[numToDraw++] = imNormal;

	// hilights etc..
	if (down)
	{
		toDraw[numToDraw++] = imDown;
	}
	if (highlight && !grey && hiToUse.images != nullptr)
	{
		toDraw[numToDraw++] = hiToUse;
	}

	for (int n = 0; n < numToDraw; ++n)
	{
		Image tcImage(toDraw[n].images, toDraw[n].id + 1);
		iV_DrawImage(toDraw[n], x0, y0);
	}

	if (grey)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}

	// TODO: Display text to the right of the checkbox image
}

static W_SCREEN* psNotificationOverlayScreen = nullptr;
static std::unique_ptr<WZ_Queued_Notification> currentNotification;
static W_NOTIFICATION* currentInGameNotification = nullptr;
static uint32_t lastNotificationClosed = 0;

bool notificationsInitialize()
{
	// TEMPORARY FOR TESTING PURPOSES:
	WZ_Notification notification;
	notification.duration = GAME_TICKS_PER_SEC * 8;
	notification.contentTitle = "Test Notification 1";
	notification.contentText = "This is a sample notification to test a very long string to see how it displays.";
	addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 30));
	notification.duration = 0;
	notification.contentTitle = "Test Notification 1 With Very Long Title That Probably Will Need to Wrap So Let's See";
	notification.contentText = "This is a sample notification to test a very long string to see how it displays.\nThis is a samble notification to test a vera lonn strinn to see how it displays.\nThere were also embedded newlines.\nDid they have an effect?\nThere were also embedded newlinesg.\nDid they have an effect?";
	addNotification(notification, WZ_Notification_Trigger::Immediate());
	notification.duration = GAME_TICKS_PER_SEC * 8;
	notification.contentTitle = "Test Notification 1";
	notification.contentText = "This is a sample notification to test a very long string to see how it displays.";
	addNotification(notification, WZ_Notification_Trigger::Immediate());

	notification.duration = GAME_TICKS_PER_SEC * 3;
	notification.contentTitle = "Test Notification 2";
	notification.contentText = "This is sample notification text to see how it displays.";
	addNotification(notification, WZ_Notification_Trigger::Immediate());
	notification.duration = 0;
	notification.contentTitle = "Update Available";
	notification.contentText = "A new version of Warzone 2100 (3.3.1) is available!";
	notification.actionButtonTitle = "Get Update Now";
	addNotification(notification, WZ_Notification_Trigger::Immediate());

	// TODO: Move this to the notifications initialization bit
	psNotificationOverlayScreen = new W_SCREEN();
	psNotificationOverlayScreen->psForm->hide(); // hiding the root form does not stop display of children, but *does* prevent it from accepting mouse over itself - i.e. basically makes it transparent
	widgRegisterOverlayScreen(psNotificationOverlayScreen, std::numeric_limits<uint16_t>::max());

	return true;
}

void notificationsShutDown()
{
	widgRemoveOverlayScreen(psNotificationOverlayScreen);
	delete psNotificationOverlayScreen;
	psNotificationOverlayScreen = nullptr;
}

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

void removeInGameNotificationForm(WZ_Queued_Notification* request)
{
	if (!request) return;

	// right now we only support a single concurrent notification
	currentInGameNotification->deleteLater();
	currentInGameNotification = nullptr;
}

#define WZ_NOTIFICATION_OPEN_DURATION (GAME_TICKS_PER_SEC*1) // Time duration for notification open/close animation
#define WZ_NOTIFICATION_CLOSE_DURATION WZ_NOTIFICATION_OPEN_DURATION
#define WZ_NOTIFICATION_TOP_PADDING 5
#define WZ_NOTIFICATION_MIN_DELAY_BETWEEN (GAME_TICKS_PER_SEC*1)

bool W_NOTIFICATION::calculateNotificationWidgetPos()//W_NOTIFICATION* psNotificationWidget)//WZ_Queued_Notification* request)
{
//	W_NOTIFICATION* psNotificationWidget = getOrCreateInGameNotificationForm(request);
	W_NOTIFICATION* psNotificationWidget = this;
	WZ_Queued_Notification* request = psNotificationWidget->request;

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
			finishedProcessingNotificationRequest(request); // after this, request is invalid!
			psNotificationWidget->request = nullptr; // TEMP
			return true; // processed notification
	}

	x += psNotificationWidget->getDragOffset().x;
	y += psNotificationWidget->getDragOffset().y;

	psNotificationWidget->move(x, y);

//	/* Process any user callback functions */
//	W_CONTEXT sContext;
//	sContext.xOffset = 0;
//	sContext.yOffset = 0;
//	sContext.mx = mouseX();
//	sContext.my = mouseY();
//	psNotificationWidget->processCallbacksRecursive(&sContext);
//
//	// Display the widgets.
//	psNotificationWidget->displayRecursive(0, 0);
//
//#if 0
//	debugBoundingBoxesOnly = true;
//	pie_SetRendMode(REND_ALPHA);
//	psNotificationWidget->displayRecursive(0, 0);
//	debugBoundingBoxesOnly = false;
//#endif

	return false;
}

/* Run a notification widget */
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
			debug(LOG_3D, "dragging up, dragOffset.y: (%d)", dragOffset.y);
		}
		else if (currMouseY > dragStartY)
		{
			// dragging down
			const int verticalLimit = 10;
			int distanceY = currMouseY - dragStartY;
			dragOffset.y = verticalLimit * (1 + log10(float(distanceY) / float(verticalLimit)));
			debug(LOG_3D, "dragging down, dragOffset.y: (%d)", dragOffset.y);
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
			debug(LOG_3D, "No longer in drag mode");
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

	calculateNotificationWidgetPos();
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
		debug(LOG_3D, "Click while closing - set to shown");
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
		debug(LOG_3D, "dragStartMousePos: (%d x %d)", dragStartMousePos.x, dragStartMousePos.y);
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
		debug(LOG_3D, "dragOffset.y: %d", dragOffset.y);
		if (!isInDragMode || dragOffset.y < WZ_NOTIFICATION_DOWN_DRAG_DISCARD_CLICK_THRESHOLD)
		{
			dismissNotification(request);
		}
	}
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

struct DisplayNotificationButtonCache
{
	WzText wzText;
};

// ////////////////////////////////////////////////////////////////////////////
// display a notification action button
void displayNotificationAction(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD			fx, fy, fw;
	W_BUTTON		*psBut = (W_BUTTON *)psWidget;
	bool			hilight = false;
	bool			greyOut = psWidget->UserData || (psBut->getState() & WBUT_DISABLE); // if option is unavailable.

	// Any widget using displayTextOption must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayNotificationButtonCache& cache = *static_cast<DisplayNotificationButtonCache*>(psWidget->pUserData);

	cache.wzText.setText(psBut->pText.toUtf8(), psBut->FontID);

	if (psBut->isHighlighted())					// if mouse is over text then hilight.
	{
		hilight = true;
	}

	fw = cache.wzText.width();
	fy = yOffset + psWidget->y() + (psWidget->height() - cache.wzText.lineSize()) / 2 - cache.wzText.aboveBase();

	if (psWidget->style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = xOffset + psWidget->x() + ((psWidget->width() - fw) / 2);
	}
	else
	{
		fx = xOffset + psWidget->x();
	}

	PIELIGHT colour;

	if (greyOut)														// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else															// available
	{
		if (hilight)													// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else														// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	cache.wzText.render(fx, fy, colour);

	return;
}

bool isDraggingInGameNotification()
{
	// right now we only support a single concurrent notification
	if (currentInGameNotification)
	{
		return currentInGameNotification->isActivelyBeingDragged();
	}
	return false;
}

W_NOTIFICATION* getOrCreateInGameNotificationForm(WZ_Queued_Notification* request)
{
	if (!request) return nullptr;

	// right now we only support a single concurrent notification
	if (!currentInGameNotification)
	{
		W_NOTIFICATION* psNewNotificationForm = nullptr;

		W_NOTIFYINIT sFormInit;
		sFormInit.formID = 0;
		sFormInit.id = 0;
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
		psNewNotificationForm = new W_NOTIFICATION(&sFormInit);
//		currentInGameNotification->setScreenPointer(psNotificationOverlayScreen);
		psNotificationOverlayScreen->psForm->attach(psNewNotificationForm);

//		/* Add the close button */
//		W_BUTINIT sButInit;
//		sButInit.formID = IDOBJ_FORM;
//		sButInit.id = IDOBJ_CLOSE;
//		sButInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
//			psWidget->setGeometry(OBJ_BACKWIDTH - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
//		});
//		sButInit.pTip = _("Close");
//		sButInit.pDisplay = intDisplayImageHilight;
//		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
//		if (!widgAddButton(psWScreen, &sButInit))
//		{
//			return false;
//		}

		// Calculate dimensions for text area
		int maxTextWidth = calculatedWidth - (WZ_NOTIFICATION_PADDING * 2) - WZ_NOTIFICATION_IMAGE_SIZE - WZ_NOTIFICATION_PADDING;

		// Add title
		W_LABEL *label_title = new W_LABEL(psNewNotificationForm);
		label_title->setGeometry(WZ_NOTIFICATION_PADDING, WZ_NOTIFICATION_PADDING, maxTextWidth, 12);
		label_title->setFontColour(WZCOL_TEXT_BRIGHT);
//		label->setString(WzString::fromUtf8(request->notification.contentTitle));
		int heightOfTitleLabel = label_title->setFormattedString(WzString::fromUtf8(request->notification.contentTitle), maxTextWidth, font_regular_bold, WZ_NOTIFICATION_CONTENTS_LINE_SPACING);
		label_title->setGeometry(label_title->x(), label_title->y(), maxTextWidth, heightOfTitleLabel);
		label_title->setTextAlignment(WLAB_ALIGNTOPLEFT);
//		label_title->setFont(font_regular_bold, WZCOL_TEXT_BRIGHT);
		label_title->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

		// Add contents
		W_LABEL *label_contents = new W_LABEL(psNewNotificationForm);
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
			sButInit.initPUserDataFunc = []() -> void * { return new DisplayNotificationButtonCache(); };
			sButInit.onDelete = [](WIDGET *psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<DisplayNotificationButtonCache *>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			};
			sButInit.pDisplay = displayNotificationAction;

	//		sButInit.height = FRONTEND_BUTHEIGHT;
	//		sButInit.pDisplay = displayTextOption;
			sButInit.FontID = font_regular_bold;
			sButInit.pText = actionLabel1.c_str();
			psButton1 = new W_BUTTON(&sButInit);
			psNewNotificationForm->attach(psButton1);
		}

		if (!actionLabel2.empty())
		{
			// Button 2
			sButInit.id = 3;
			sButInit.width = iV_GetTextWidth(actionLabel2.c_str(), font_regular_bold) + 15;
			sButInit.x = (short)(psButton1->x() - 15 - sButInit.width);
			sButInit.pText = actionLabel2.c_str();
			psButton2 = new W_BUTTON(&sButInit);
			psNewNotificationForm->attach(psButton2);
		}

		if (request->notification.onDoNotShowAgain)
		{
			// TODO: display "do not show again" button with checkbox image
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
		psNewNotificationForm->setGeometry(psNewNotificationForm->x(), psNewNotificationForm->y(), psNewNotificationForm->width(), calculatedHeight);

		currentInGameNotification = psNewNotificationForm;
	}
	return currentInGameNotification;
}


void displayNotificationInGame(WZ_Queued_Notification* request)
{
	ASSERT(request, "request is null");
	getOrCreateInGameNotificationForm(request);
	// NOTE: Can ignore the result of the above, because it automatically attaches it to the root notification overlay screen
}

void runNotifications()
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
		if (currentNotification)
		{
			displayNotificationInGame(currentNotification.get());
		}
	}
//	if (!currentNotification)
//	{
//		return;
//	}
//
//

//	if (displayNotificationInGame(currentNotification.get()))
//	{
//		// finished with this notification
//		currentNotification.reset();
//		lastNotificationClosed = realTime;
//	}
}

bool notifications_processClickRecursive(int32_t mx, int32_t my, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	sContext.mx = mx;
	sContext.my = my;

	if (currentInGameNotification && currentInGameNotification->globalHitTest(sContext.mx, sContext.my))
	{
		// Forward first to in-game notifications, which are above everything else
		return currentInGameNotification->processClickRecursive(&sContext, key, wasPressed);
	}
	return false;
}

void finishedProcessingNotificationRequest(WZ_Queued_Notification* request)
{
	// at the moment, we only support processing a single notification at a time

	// finished with this notification
	currentNotification.reset(); // at this point request is no longer valid!
	lastNotificationClosed = realTime;
}

void addNotification(const WZ_Notification& notification, const WZ_Notification_Trigger& trigger)
{
	notificationQueue.push_back(std::unique_ptr<WZ_Queued_Notification>(new WZ_Queued_Notification(notification, WZ_Notification_Status(realTime), trigger)));
//	notificationQueue.back().get()->setProcessedNotificationRequestCallback([](WZ_Queued_Notification *request) {
//		finishedProcessingNotificationRequest(request);
//	});
}

// New in-game notification system
