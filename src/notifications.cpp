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
#include "init.h"
#include "frend.h"

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
	float animationSpeed = 1.0f;
public:
	// normal speed is 1.0
	void setAnimationSpeed(float newSpeed)
	{
		animationSpeed = newSpeed;
	}
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
	void setState(WZ_Notification_Status::NotificationState newState);

public:
	WZ_Notification notification;
	WZ_Notification_Status status;
	WZ_Notification_Trigger trigger;
private:
	ProcessedRequestCallback onProcessedNotificationRequestFunc;
	bool bWasFullyShown = false;
};

// MARK: - Notification Ignore List

#include <3rdparty/json/json.hpp>
using json = nlohmann::json;

#include <typeinfo>

class WZ_Notification_Preferences
{
public:
	WZ_Notification_Preferences(const std::string &fileName);

	void incrementNotificationRuns(const std::string& uniqueNotificationIdentifier);
	uint32_t getNotificationRuns(const std::string& uniqueNotificationIdentifier) const;

	void doNotShowNotificationAgain(const std::string& uniqueNotificationIdentifier);
	bool getDoNotShowNotificationValue(const std::string& uniqueNotificationIdentifier) const;
	bool canShowNotification(const WZ_Notification& notification) const;

	void clearAllNotificationPreferences();

	bool savePreferences();

private:
	void storeValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& value);
	json getJSONValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& defaultValue = json()) const;

	template<typename T>
	T getValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, T defaultValue) const
	{
		T result = defaultValue;
		try {
			result = getJSONValueForNotification(uniqueNotificationIdentifier, key, json(defaultValue)).get<T>();
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert json_variant to %s because of error: %s", typeid(T).name(), e.what());
			result = defaultValue;
		}
		catch (...) {
			debug(LOG_FATAL, "Unexpected exception encountered: json_variant::toType<%s>", typeid(T).name());
		}
		return result;
	}

private:
	json mRoot;
	std::string mFilename;
};

#include <physfs.h>
#include "lib/framework/file.h"
#include <sstream>

WZ_Notification_Preferences::WZ_Notification_Preferences(const std::string &fileName)
: mFilename(fileName)
{
	if (!PHYSFS_exists(fileName.c_str()))
	{
		// no preferences exist yet
		return;
	}

	UDWORD size;
	char *data;
	if (!loadFile(fileName.c_str(), &data, &size))
	{
		debug(LOG_ERROR, "Could not open \"%s\"", fileName.c_str());
		// treat as if no preferences exist yet
		return;
	}

	try {
		mRoot = json::parse(data, data + size);
	}
	catch (const std::exception &e) {
		ASSERT(false, "JSON document from %s is invalid: %s", fileName.c_str(), e.what());
	}
	catch (...) {
		debug(LOG_ERROR, "Unexpected exception parsing JSON %s", fileName.c_str());
	}
	ASSERT(!mRoot.is_null(), "JSON document from %s is null", fileName.c_str());
	ASSERT(mRoot.is_object(), "JSON document from %s is not an object. Read: \n%s", fileName.c_str(), data);
	free(data);

	// always ensure there's a "notifications" dictionary in the root object
	auto notificationsObject = mRoot.find("notifications");
	if (notificationsObject == mRoot.end() || !notificationsObject->is_object())
	{
		// create a dictionary object
		mRoot["notifications"] = json::object();
	}
}

void WZ_Notification_Preferences::storeValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& value)
{
	json& notificationsObj = mRoot["notifications"];
	auto notificationData = notificationsObj.find(uniqueNotificationIdentifier);
	if (notificationData == notificationsObj.end() || !notificationData->is_object())
	{
		notificationsObj[uniqueNotificationIdentifier] = json::object();
		notificationData = notificationsObj.find(uniqueNotificationIdentifier);
	}
	(*notificationData)[key] = value;
}

json WZ_Notification_Preferences::getJSONValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& defaultValue /*= json()*/) const
{
	try {
		return mRoot.at("notifications").at(uniqueNotificationIdentifier).at(key);
	}
	catch (json::out_of_range&)
	{
		// some part of the path doesn't exist yet - return the default value
		return defaultValue;
	}
}

void WZ_Notification_Preferences::incrementNotificationRuns(const std::string& uniqueNotificationIdentifier)
{
	uint32_t seenCount = getNotificationRuns(uniqueNotificationIdentifier);
	storeValueForNotification(uniqueNotificationIdentifier, "seen", ++seenCount);
}

uint32_t WZ_Notification_Preferences::getNotificationRuns(const std::string& uniqueNotificationIdentifier) const
{
	return getValueForNotification(uniqueNotificationIdentifier, "seen", 0);
}

void WZ_Notification_Preferences::doNotShowNotificationAgain(const std::string& uniqueNotificationIdentifier)
{
	storeValueForNotification(uniqueNotificationIdentifier, "skip", true);
}

bool WZ_Notification_Preferences::getDoNotShowNotificationValue(const std::string& uniqueNotificationIdentifier) const
{
	return getValueForNotification(uniqueNotificationIdentifier, "skip", false);
}

void WZ_Notification_Preferences::clearAllNotificationPreferences()
{
	mRoot["notifications"] = json::object();
}

bool WZ_Notification_Preferences::savePreferences()
{
	std::ostringstream stream;
	stream << mRoot.dump(4) << std::endl;
	std::string jsonString = stream.str();
	saveFile(mFilename.c_str(), jsonString.c_str(), jsonString.size());
	return true;
}

bool WZ_Notification_Preferences::canShowNotification(const WZ_Notification& notification) const
{
	if (notification.displayOptions.uniqueNotificationIdentifier().empty())
	{
		return true;
	}

	bool suppressNotification = false;
	if (notification.displayOptions.isOneTimeNotification())
	{
		suppressNotification = (getNotificationRuns(notification.displayOptions.uniqueNotificationIdentifier()) > 0);
	}
	else
	{
		suppressNotification = getDoNotShowNotificationValue(notification.displayOptions.uniqueNotificationIdentifier());
	}

	return !suppressNotification;
}

static WZ_Notification_Preferences* notificationPrefs = nullptr;

void WZ_Queued_Notification::setState(WZ_Notification_Status::NotificationState newState)
{
	status.state = newState;
	status.stateStartTime = realTime;

	if (newState == WZ_Notification_Status::NotificationState::closed)
	{
		if (notification.isIgnoreable() && !bWasFullyShown)
		{
			notificationPrefs->incrementNotificationRuns(notification.displayOptions.uniqueNotificationIdentifier());
		}
		bWasFullyShown = true;
	}
}


// MARK: - TEMPORARY
void finishedProcessingNotificationRequest(WZ_Queued_Notification* request, bool doNotShowAgain);

//

// MARK: -

static std::list<std::unique_ptr<WZ_Queued_Notification>> notificationQueue;

std::unique_ptr<WZ_Queued_Notification> popNextQueuedNotification()
{
	ASSERT(notificationPrefs, "Notification preferences not loaded!");
	auto it = notificationQueue.begin();
	while (it != notificationQueue.end())
	{
		auto & request = *it;

		if (!notificationPrefs->canShowNotification(request->notification))
		{
			// ignore this notification - remove from the list
			debug(LOG_ERROR, "Ignoring notification: %s", request->notification.displayOptions.uniqueNotificationIdentifier().c_str());
			it = notificationQueue.erase(it);
			continue;

//			bool skipNotification = false;
//			if (request->notification.displayOptions.isOneTimeNotification())
//			{
//				debug(LOG_ERROR, "Ignorning one-time notification: %s", request->notification.displayOptions.uniqueNotificationIdentifier().c_str());
//				skipNotification = (notificationPrefs->getNotificationRuns(request->notification.displayOptions.uniqueNotificationIdentifier()) > 0);
//			}
//			else
//			{
//				skipNotification = !notificationPrefs->canShowNotification(request->notification.displayOptions.uniqueNotificationIdentifier());
//			}
//
//			if (skipNotification)
//			{
//				// ignore this notification - remove from the list
//				debug(LOG_ERROR, "Ignoring notification: %s", request->notification.displayOptions.uniqueNotificationIdentifier().c_str());
//				it = notificationQueue.erase(it);
//				continue;
//			}
		}

		uint32_t num = std::min<uint32_t>(realTime - request->status.queuedTime, request->trigger.timeInterval);
		if (num == request->trigger.timeInterval)
		{
			std::unique_ptr<WZ_Queued_Notification> retVal(std::move(request));
			it = notificationQueue.erase(it);
			return retVal;
		}

		++it;
	}
	return nullptr;
}

#include "lib/framework/input.h"

/** Notification initialisation structure */

struct WzCheckboxButton;

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
	void run(W_CONTEXT *psContext) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
public:
//	bool globalHitTest(int x, int y) { return hitTest(x, y); }
	Vector2i getDragOffset() const { return dragOffset; }
	bool isActivelyBeingDragged() const { return isInDragMode; }
	uint32_t getLastDragStartTime() const { return dragStartedTime; }
	uint32_t getLastDragEndTime() const { return dragEndedTime; }
	uint32_t getLastDragDuration() const { return dragEndedTime - dragStartedTime; }
private:
	bool calculateNotificationWidgetPos();
	gfx_api::texture* loadImage(const std::string& filename);
public:
	WzCheckboxButton *pOnDoNotShowAgainCheckbox = nullptr;
private:
	WZ_Queued_Notification* request;
	bool DragEnabled = true;
	bool isInDragMode = false;
	Vector2i dragOffset = {0, 0};
	Vector2i dragStartMousePos = {0, 0};
	Vector2i dragOffsetEnded = {0, 0};
	uint32_t dragStartedTime = 0;
	uint32_t dragEndedTime = 0;
public: // TEMP
	gfx_api::texture* pImageTexture = nullptr;
};

W_NOTIFICATION::W_NOTIFICATION(W_NOTIFYINIT const *init)
: W_FORM(init)
, request(init->request)
{
	// TODO:

	// Load image, if specified
	if (!request->notification.largeIconPath.empty())
	{
		pImageTexture = loadImage(request->notification.largeIconPath);
//		pImageTexture = new GFX(GFX_TEXTURE, GL_TRIANGLE_STRIP, 2);
//
//		pImageTexture->loadTexture(request->notification.largeIconPath.c_str()); // TODO: Handle failure?

//		gfx_api::gfxFloat x1 = 0, x2 = screenWidth, y1 = 0, y2 = screenHeight;
//		gfx_api::gfxFloat tx = 1, ty = 1;
////		int scale = 0, w = 0, h = 0;
////		const float aspect = screenWidth / (float)screenHeight, backdropAspect = 4 / (float)3;
////
////		if (aspect < backdropAspect)
////		{
////			int offset = (screenWidth - screenHeight * backdropAspect) / 2;
////			x1 += offset;
////			x2 -= offset;
////		}
////		else
////		{
////			int offset = (screenHeight - screenWidth / backdropAspect) / 2;
////			y1 += offset;
////			y2 -= offset;
////		}
//
////		if (backdropIsMapPreview) // preview
////		{
////			int s1 = screenWidth / preview_width;
////			int s2 = screenHeight / preview_height;
////			scale = MIN(s1, s2);
////
////			w = preview_width * scale;
////			h = preview_height * scale;
////			x1 = screenWidth / 2 - w / 2;
////			x2 = screenWidth / 2 + w / 2;
////			y1 = screenHeight / 2 - h / 2;
////			y2 = screenHeight / 2 + h / 2;
////
////			tx = preview_width / (float)BACKDROP_HACK_WIDTH;
////			ty = preview_height / (float)BACKDROP_HACK_HEIGHT;
////		}
//
//		// Generate coordinates and put them into VBOs
//		gfx_api::gfxFloat texcoords[8] = { 0.0f, 0.0f,  tx, 0.0,  0.0f, ty,  tx, ty };
//		gfx_api::gfxFloat vertices[8] = { x1, y1,  x2, y1,  x1, y2,  x2, y2 };
//		pImageTexture->buffers(4, vertices, texcoords);
	}
}

#include "lib/ivis_opengl/piestate.h"

gfx_api::texture* makeTexture(int width, int height, GLenum filter, const gfx_api::pixel_format& format, const GLvoid *image)
{
	pie_SetTexturePage(TEXPAGE_EXTERN);
	gfx_api::texture* mTexture = gfx_api::context::get().create_texture(width, height, format);
	if (image != nullptr)
		mTexture->upload(0u, 0u, 0u, width, height, format, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	mWidth = width;
//	mHeight = height;
//	mFormat = format;
	return mTexture;
}

gfx_api::texture* W_NOTIFICATION::loadImage(const std::string &filename)
{
	gfx_api::texture* pTexture = nullptr;
	const char *extension = strrchr(filename.c_str(), '.'); // determine the filetype
	iV_Image image;
	if (!extension || strcmp(extension, ".png") != 0)
	{
		debug(LOG_ERROR, "Bad image filename: %s", filename.c_str());
		return nullptr;
	}
	if (iV_loadImage_PNG(filename.c_str(), &image))
	{
		pTexture = makeTexture(image.width, image.height, GL_LINEAR, iV_getPixelFormat(&image), image.bmp);
		iV_unloadImage(&image);
	}
	return pTexture;
}


struct WzCheckboxButton : public W_BUTTON
{
public:
	typedef std::function<void (WzCheckboxButton& button, bool isChecked)> W_CHECKBOX_ONCHECK_CHANGE_FUNC;

public:
	WzCheckboxButton(WIDGET *parent) : W_BUTTON(parent)
	{
		addOnClickHandler([](W_BUTTON& button) {
			WzCheckboxButton& self = static_cast<WzCheckboxButton&>(button);
			self.isChecked = !self.isChecked;
		});
	}

	void display(int xOffset, int yOffset);

	Image imNormal;
	Image imDown;
	unsigned doHighlight;

	Vector2i calculateDesiredDimensions();

	void setOnCheckStateChanged(const W_CHECKBOX_ONCHECK_CHANGE_FUNC& handler)
	{
		onCheckStateChanged = handler;
	}

	bool getIsChecked() const { return isChecked; }
private:
	int checkboxSize()
	{
		wzText.setText(pText.toUtf8(), FontID);
		return wzText.lineSize() - 2; //std::min({width(), height(), wzText.lineSize()});
	}
private:
	WzText wzText;
	bool isChecked = false;
	W_CHECKBOX_ONCHECK_CHANGE_FUNC onCheckStateChanged;
};

Image mpwidgetGetFrontHighlightImage_2(Image image) //TODO: Move the original somewhere accessible?
{
	if (image.isNull())
	{
		return Image();
	}
	switch (image.width())
	{
	case 30: return Image(FrontImages, IMAGE_HI34);
	case 60: return Image(FrontImages, IMAGE_HI64);
	case 19: return Image(FrontImages, IMAGE_HI23);
	case 27: return Image(FrontImages, IMAGE_HI31);
	case 35: return Image(FrontImages, IMAGE_HI39);
	case 37: return Image(FrontImages, IMAGE_HI41);
	case 56: return Image(FrontImages, IMAGE_HI56);
	}
	return Image();
}

//void WzCheckboxButton::display(int xOffset, int yOffset)
//{
//	int x0 = xOffset + x();
//	int y0 = yOffset + y();
//	Image hiToUse(nullptr, 0);
//
//	// evaluate auto-frame
//	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;
//
//	// evaluate auto-frame
//	if (doHighlight == 1 && highlight)
//	{
//		hiToUse = mpwidgetGetFrontHighlightImage_2(imNormal);
//	}
//
//	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
//	bool grey = (getState() & WBUT_DISABLE) != 0;
//
//	Image toDraw[3];
//	int numToDraw = 0;
//
//	// now display
//	toDraw[numToDraw++] = imNormal;
//
//	// hilights etc..
//	if (down)
//	{
//		toDraw[numToDraw++] = imDown;
//	}
//	if (highlight && !grey && hiToUse.images != nullptr)
//	{
//		toDraw[numToDraw++] = hiToUse;
//	}
//
//	for (int n = 0; n < numToDraw; ++n)
//	{
//		Image tcImage(toDraw[n].images, toDraw[n].id + 1);
//		iV_DrawImage(toDraw[n], x0, y0);
//	}
//
//	if (grey)
//	{
//		// disabled, render something over it!
//		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
//	}
//
//	// TODO: Display text to the right of the checkbox image
//}

Vector2i WzCheckboxButton::calculateDesiredDimensions()
{
	int cbSize = checkboxSize();
	Vector2i checkboxPos{x(), y()};
	int textLeftPos = checkboxPos.x + cbSize + 7;

	// TODO: Incorporate padding?
	return Vector2i(textLeftPos + wzText.width(), std::max(wzText.lineSize(), cbSize));
}

void WzCheckboxButton::display(int xOffset, int yOffset)
{
	wzText.setText(pText.toUtf8(), FontID);

	int x0 = xOffset + x();
	int y0 = yOffset + y();
	Image hiToUse(nullptr, 0);

//	// evaluate auto-frame
//	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;
//
//	// evaluate auto-frame
//	if (doHighlight == 1 && highlight)
//	{
//		hiToUse = mpwidgetGetFrontHighlightImage_2(imNormal);
//	}

	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
//	bool grey = (getState() & WBUT_DISABLE) != 0;

//	Image toDraw[3];
//	int numToDraw = 0;
//
//	// now display
//	toDraw[numToDraw++] = imNormal;
//
//	// hilights etc..
//	if (down)
//	{
//		toDraw[numToDraw++] = imDown;
//	}
//	if (highlight && !grey && hiToUse.images != nullptr)
//	{
//		toDraw[numToDraw++] = hiToUse;
//	}
//
//	for (int n = 0; n < numToDraw; ++n)
//	{
//		Image tcImage(toDraw[n].images, toDraw[n].id + 1);
//		iV_DrawImage(toDraw[n], x0, y0);
//	}

	// calculate checkbox dimensions
	int cbSize = checkboxSize(); //std::min({width(), height(), wzText.lineSize()});
	Vector2i checkboxOffset{0, (height() - cbSize) / 2}; // left-align, center vertically
	Vector2i checkboxPos{x0 + checkboxOffset.x, y0 + checkboxOffset.y};

	// draw checkbox border
	PIELIGHT notifyBoxAddColor = WZCOL_NOTIFICATION_BOX;
	notifyBoxAddColor.byte.a = uint8_t(float(notifyBoxAddColor.byte.a) * 0.7f);
	pie_UniTransBoxFill(checkboxPos.x, checkboxPos.y, checkboxPos.x + cbSize, checkboxPos.y + cbSize, notifyBoxAddColor);
	iV_Box2(checkboxPos.x, checkboxPos.y, checkboxPos.x + cbSize, checkboxPos.y + cbSize, WZCOL_TEXT_MEDIUM, WZCOL_TEXT_MEDIUM);

	if (down || isChecked)
	{
		// draw checkbox "x" mark
		#define CB_INNER_INSET 2
		PIELIGHT checkBoxInsideColor = WZCOL_TEXT_MEDIUM;
		checkBoxInsideColor.byte.a = 200;
		pie_UniTransBoxFill(checkboxPos.x + CB_INNER_INSET, checkboxPos.y + CB_INNER_INSET, checkboxPos.x + cbSize - (CB_INNER_INSET), checkboxPos.y + cbSize - (CB_INNER_INSET), checkBoxInsideColor);
	}

//	if (grey)
	if (false)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}

	// TODO: Display text to the right of the checkbox image
	int textLeftPos = checkboxPos.x + cbSize + 7;


	int fx = textLeftPos;
	int fw = wzText.width();
	int fy = yOffset + y() + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();

	if (style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = /*xOffset + x()*/ textLeftPos + ((width() - fw) / 2);
	}
	else
	{
		fx = /*xOffset + x()*/ textLeftPos;
	}

	wzText.render(fx, fy, WZCOL_TEXT_MEDIUM);
}

static W_SCREEN* psNotificationOverlayScreen = nullptr;
static std::unique_ptr<WZ_Queued_Notification> currentNotification;
static W_NOTIFICATION* currentInGameNotification = nullptr;
static uint32_t lastNotificationClosed = 0;

bool notificationsInitialize()
{
	notificationPrefs = new WZ_Notification_Preferences("notifications.json");

	// TEMPORARY FOR TESTING PURPOSES:
	WZ_Notification notification;
	notification.duration = GAME_TICKS_PER_SEC * 8;
	notification.contentTitle = "Test Notification 1";
	notification.contentText = "This is a sample notification to test a very long string to see how it displays.";
	addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 30));
	notification.duration = 0;
	notification.contentTitle = "Test Notification 1 With Very Long Title That Probably Will Need to Wrap So Let's See";
	notification.contentText = "This is a sample notification to test a very long string to see how it displays.\nThis is a samble notification to test a vera lonn strinn to see how it displays.\nThere were also embedded newlines.\nDid they have an effect?\nThere were also embedded newlinesg.\nDid they have an effect?";
	notification.largeIconPath = "images/warzone2100.png";
	addNotification(notification, WZ_Notification_Trigger::Immediate());

	notification.largeIconPath.clear();

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
	notification.action = WZ_Notification_Action("Get Update Now", [](const WZ_Notification&){
		// Example: open a URL
		debug(LOG_ERROR, "Get Update Now action clicked");
	});
//	notification.onDoNotShowAgain = [](WZ_Notification& notification) {
//		debug(LOG_ERROR, "Do not show again");
//	};
	notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable("wz_new_version_3_3_1_b", 3);
	addNotification(notification, WZ_Notification_Trigger::Immediate());

	// TODO: Move this to the notifications initialization bit
	psNotificationOverlayScreen = new W_SCREEN();
	psNotificationOverlayScreen->psForm->hide(); // hiding the root form does not stop display of children, but *does* prevent it from accepting mouse over itself - i.e. basically makes it transparent
	widgRegisterOverlayScreen(psNotificationOverlayScreen, std::numeric_limits<uint16_t>::max());

	return true;
}

void notificationsShutDown()
{
	notificationPrefs->savePreferences();
	delete notificationPrefs;
	notificationPrefs = nullptr;

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

void dismissNotification(WZ_Queued_Notification* request, float animationSpeed = 1.0f)
{
	// if notification is the one being displayed, animate it away by setting its state to closing
	switch (request->status.state)
	{
		case WZ_Notification_Status::NotificationState::submitted:
			// should not happen
			break;
////		case WZ_Notification_Status::NotificationState::opening:
////			request->status.state = WZ_Notification_Status::NotificationState::shown;
////			request->status.stateStartTime = realTime;
//			request->setState(WZ_Notification_Status::NotificationState::shown);
//			break;
		case WZ_Notification_Status::NotificationState::shown:
//			request->status.state = WZ_Notification_Status::NotificationState::closing;
//			request->status.stateStartTime = realTime;
			request->status.setAnimationSpeed(animationSpeed);
			request->setState(WZ_Notification_Status::NotificationState::closing);
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
#define WZ_NOTIFICATION_CLOSE_DURATION (WZ_NOTIFICATION_OPEN_DURATION / 2)
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
//			request->status.state = WZ_Notification_Status::NotificationState::opening;
//			request->status.stateStartTime = realTime;
			request->setState(WZ_Notification_Status::NotificationState::opening);
			// fall-through
		case WZ_Notification_Status::NotificationState::opening:
		{
			// calculate how far we are on opening based on the stateStartTime
			uint32_t startTime = request->status.stateStartTime;
//			if (!psNotificationWidget->isActivelyBeingDragged() && psNotificationWidget->getLastDragStartTime() > startTime)
//			{
//				startTime += psNotificationWidget->getLastDragEnded();
//			}
			float openAnimationDuration = float(WZ_NOTIFICATION_OPEN_DURATION) * request->status.animationSpeed;
			uint32_t endTime = startTime + uint32_t(openAnimationDuration);
			if (realTime < endTime)
			{
//				y = startingOffscreenY;
//				uint32_t
//				y += ((float(realTime) / float(endTime)) * WZ_NOTIFICATION_OPEN_DURATION)
//				y = easeChange(float(realTime) / float(endTime), startingOffscreenY, WZ_NOTIFICATION_TOP_PADDING, float(1.0));
//				y = (-psNotificationWidget->height()) + (BezierBlend((float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_OPEN_DURATION)) * (WZ_NOTIFICATION_TOP_PADDING + psNotificationWidget->height()));
				y = (-psNotificationWidget->height()) + (QuinticEaseOut((float(realTime) - float(startTime)) / float(openAnimationDuration)) * (endingYPosition + psNotificationWidget->height()));
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
//			request->status.state = WZ_Notification_Status::NotificationState::shown;
//			request->status.stateStartTime = realTime;
			request->setState(WZ_Notification_Status::NotificationState::shown);
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
//			request->status.state = WZ_Notification_Status::NotificationState::closing;
//			request->status.stateStartTime = realTime;
			request->setState(WZ_Notification_Status::NotificationState::closing);
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
			float closeAnimationDuration = float(WZ_NOTIFICATION_CLOSE_DURATION) * request->status.animationSpeed;
			uint32_t endTime = startTime + uint32_t(closeAnimationDuration);
			if (realTime < endTime)
			{
//				y = easeOut(float(realTime) / float(endTime), WZ_NOTIFICATION_TOP_PADDING, startingOffscreenY, float(WZ_NOTIFICATION_CLOSE_DURATION / GAME_TICKS_PER_SEC));
//				y = WZ_NOTIFICATION_TOP_PADDING - (ElasticEaseIn((float(realTime) - float(startTime)) / float(WZ_NOTIFICATION_OPEN_DURATION)) * (WZ_NOTIFICATION_TOP_PADDING + psNotificationWidget->height()));
				float percentComplete = (float(realTime) - float(startTime)) / float(closeAnimationDuration);
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
//			request->status.state = WZ_Notification_Status::NotificationState::closed;
//			request->status.stateStartTime = realTime;
			request->setState(WZ_Notification_Status::NotificationState::closed);
			// fall-through
		}
		case WZ_Notification_Status::NotificationState::closed:
			// widget is now off-screen - get checkbox state (if present)
			bool bDoNotShowAgain = false;
			if (pOnDoNotShowAgainCheckbox)
			{
				bDoNotShowAgain = pOnDoNotShowAgainCheckbox->getIsChecked();
			}
			// then destroy the widget, and finalize the notification request
			removeInGameNotificationForm(request);
			finishedProcessingNotificationRequest(request, bDoNotShowAgain); // after this, request is invalid!
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
#define WZ_NOTIFICATION_IMAGE_SIZE	36
#define WZ_NOTIFICATION_CONTENTS_LINE_SPACING	0
#define WZ_NOTIFICATION_CONTENTS_TOP_PADDING	5
#define WZ_NOTIFICATION_BUTTON_HEIGHT 20
#define WZ_NOTIFICATION_BETWEEN_BUTTON_PADDING	10

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include "lib/ivis_opengl/pieblitfunc.h"

static void displayNotificationWidget(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
//	iV_DrawImage2(WzString::fromUtf8("image_fe_logo.png"), xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());
	W_NOTIFICATION *psNotification = static_cast<W_NOTIFICATION*>(psWidget);

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

	pie_UniTransBoxFill(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 50));
	pie_UniTransBoxFill(x0, y0, x1, y1, pal_RGBA(0, 0, 0, 50));

//	debug(LOG_3D, "Draw notification widget: (%d x %d), (width: %d, height: %d) => (%dx%d),(%dx%d)", psWidget->x(), psWidget->y(), psWidget->width(), psWidget->height(), x0, y0, x1, y1);
//	int shadowPadding = 2;
//	pie_UniTransBoxFill(x0 + shadowPadding, y0 + shadowPadding, x1 + shadowPadding, y1 + shadowPadding, pal_RGBA(0, 0, 0, 70));
	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_NOTIFICATION_BOX);
	iV_Box2(x0, y0, x1, y1, WZCOL_FORM_DARK, WZCOL_FORM_DARK);
	iV_Box2(x0 - 1, y0 - 1, x1 + 1, y1 + 1, pal_RGBA(255, 255, 255, 50), pal_RGBA(255, 255, 255, 50));

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
//	iV_Box2(imageLeft, imageTop, imageLeft + WZ_NOTIFICATION_IMAGE_SIZE, imageTop + WZ_NOTIFICATION_IMAGE_SIZE, WZCOL_RED, WZCOL_RED);

	if (psNotification->pImageTexture)
	{
		int image_x0 = imageLeft;
		int image_y0 = imageTop;
//		int image_x1 = imageLeft + WZ_NOTIFICATION_IMAGE_SIZE;
//		int image_y1 = imageTop + WZ_NOTIFICATION_IMAGE_SIZE;
//		if (image_x0 > x1)
//		{
//			std::swap(x0, x1);
//		}
//		if (y0 > y1)
//		{
//			std::swap(y0, y1);
//		}
//		const auto& center = Vector2f(image_x0, image_y0);
//		const auto& mvp = defaultProjectionMatrix() * glm::translate(Vector3f(center, 0.f)) * glm::scale(glm::vec3(image_x1 - image_x0, image_y1 - image_y0, 1.f));

		iV_DrawImage(psNotification->pImageTexture->id(), Vector2i(image_x0, image_y0), Vector2f(0,0), Vector2f(WZ_NOTIFICATION_IMAGE_SIZE, WZ_NOTIFICATION_IMAGE_SIZE), 0.f, REND_ALPHA, WZCOL_WHITE);
//		psNotification->pImageTexture->draw(mvp); //glm::ortho(0.f, (float)pie_GetVideoBufferWidth(), (float)pie_GetVideoBufferHeight(), 0.f));
	}
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
	bool			greyOut = /*psWidget->UserData ||*/ (psBut->getState() & WBUT_DISABLE); // if option is unavailable.
	bool			isActionButton = (psBut->UserData == 1);

	// Any widget using displayTextOption must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayNotificationButtonCache& cache = *static_cast<DisplayNotificationButtonCache*>(psWidget->pUserData);

	cache.wzText.setText(psBut->pText.toUtf8(), psBut->FontID);

	if (psBut->isHighlighted())					// if mouse is over text then hilight.
	{
		hilight = true;
	}

	PIELIGHT colour;

	if (greyOut)														// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else															// available
	{
		if (hilight || isActionButton)													// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else														// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	if (isActionButton)
	{
		// "Action" buttons have a bordering box
		int x0 = psBut->x() + xOffset;
		int y0 = psBut->y() + yOffset;
		int x1 = x0 + psBut->width();
		int y1 = y0 + psBut->height();
//		PIELIGHT fillClr = pal_RGBA(0, 0, 0, 10);
		if (hilight)
		{
			PIELIGHT fillClr = pal_RGBA(255, 255, 255, 30);
			pie_UniTransBoxFill(x0, y0, x1, y1, fillClr);
		}
		iV_Box(x0, y0, x1, y1, colour);
//		iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, /*isDisabled ? WZCOL_FORM_LIGHT : */ WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
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

	if (!greyOut)
	{
		cache.wzText.render(fx + 1, fy + 1, pal_RGBA(0, 0, 0, 80));
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

#define WZ_NOTIFY_DONOTSHOWAGAINCB_ID 5

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
		int imageSize = (psNewNotificationForm->pImageTexture) ? WZ_NOTIFICATION_IMAGE_SIZE : 0;
		int maxTextWidth = calculatedWidth - (WZ_NOTIFICATION_PADDING * 2) - imageSize - ((imageSize > 0) ? WZ_NOTIFICATION_PADDING : 0);

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

		// Add action buttons
		std::string dismissLabel = _("Dismiss");
		std::string actionLabel = request->notification.action.title;
//		if (request->notification.duration > 0)
//		{
//			actionLabel1 = request->notification.actionButtonTitle;
//		}
//		else
//		{
//			actionLabel2 = request->notification.actionButtonTitle;
//		}

		W_BUTTON *psActionButton = nullptr;
		W_BUTTON *psDismissButton = nullptr;

		// Position the buttons below the text contents area
		int buttonsTop = label_contents->y() + label_contents->height() + WZ_NOTIFICATION_PADDING;

		W_BUTINIT sButInit;
		sButInit.formID = 0;
		sButInit.height = WZ_NOTIFICATION_BUTTON_HEIGHT;
		sButInit.y = buttonsTop;
		sButInit.style |= WBUT_TXTCENTRE;
		sButInit.UserData = 0; // store whether bordered or not
		sButInit.initPUserDataFunc = []() -> void * { return new DisplayNotificationButtonCache(); };
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayNotificationButtonCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};
		sButInit.pDisplay = displayNotificationAction;

		if (!actionLabel.empty())
		{
			// Display both an "Action" button and a "Dismiss" button

			// 1.) "Action" button
//			sButInit.formID = 0;
			sButInit.id = 2;
			sButInit.width = iV_GetTextWidth(actionLabel.c_str(), font_regular_bold) + 18;
//			sButInit.height = WZ_NOTIFICATION_BUTTON_HEIGHT;
			sButInit.x = (short)(sFormInit.width - WZ_NOTIFICATION_PADDING - sButInit.width);
//			sButInit.y = buttonsTop;
//			sButInit.style |= WBUT_TXTCENTRE;

			sButInit.UserData = 1; // store "action" state
//			sButInit.initPUserDataFunc = []() -> void * { return new DisplayNotificationButtonCache(); };
//			sButInit.onDelete = [](WIDGET *psWidget) {
//				assert(psWidget->pUserData != nullptr);
//				delete static_cast<DisplayNotificationButtonCache *>(psWidget->pUserData);
//				psWidget->pUserData = nullptr;
//			};
//			sButInit.pDisplay = displayNotificationAction;

	//		sButInit.height = FRONTEND_BUTHEIGHT;
	//		sButInit.pDisplay = displayTextOption;
			sButInit.FontID = font_regular_bold;
			sButInit.pText = actionLabel.c_str();
			psActionButton = new W_BUTTON(&sButInit);
			psActionButton->addOnClickHandler([request](W_BUTTON& button) {
				if (request->notification.action.onAction)
				{
					request->notification.action.onAction(request->notification);
				}
				else
				{
					debug(LOG_ERROR, "Action defined (\"%s\"), but no action handler!", request->notification.action.title.c_str());
				}
				dismissNotification(request);
			});
			psNewNotificationForm->attach(psActionButton);
		}

		if (psActionButton != nullptr || request->notification.duration == 0)
		{
			// 2.) "Dismiss" button
			dismissLabel = u8"▴ " + dismissLabel;
			sButInit.id = 3;
			sButInit.FontID = font_regular;
			sButInit.width = iV_GetTextWidth(dismissLabel.c_str(), font_regular) + 18;
			sButInit.x = (short)(((psActionButton) ? (psActionButton->x()) - WZ_NOTIFICATION_BETWEEN_BUTTON_PADDING : sFormInit.width - WZ_NOTIFICATION_PADDING) - sButInit.width);
			sButInit.pText = dismissLabel.c_str();
			sButInit.UserData = 0; // store regular state
			psDismissButton = new W_BUTTON(&sButInit);
			psDismissButton->addOnClickHandler([request](W_BUTTON& button) {
				dismissNotification(request);
			});
			psNewNotificationForm->attach(psDismissButton);
		}

		if (request->notification.isIgnoreable() && !request->notification.displayOptions.isOneTimeNotification())
		{
			ASSERT(notificationPrefs, "Notification preferences not loaded!");
			auto numTimesShown = notificationPrefs->getNotificationRuns(request->notification.displayOptions.uniqueNotificationIdentifier());
			if (numTimesShown >= request->notification.displayOptions.numTimesSeenBeforeDoNotShowAgainOption())
			{
				// Display "do not show again" button with checkbox
				WzCheckboxButton *pDoNotShowAgainButton = new WzCheckboxButton(psNewNotificationForm);
	//			pDoNotShowAgainButton->imNormal = Image(FrontImages, IMAGE_CHECK_OFF);
	//			pDoNotShowAgainButton->imDown = Image(FrontImages, IMAGE_CHECK_ON);
	//			int maxImageWidth = std::max(pDoNotShowAgainButton->imNormal.width(), pDoNotShowAgainButton->imDown.width());
	//			int maxImageHeight = std::max(pDoNotShowAgainButton->imNormal.height(), pDoNotShowAgainButton->imDown.height());
				pDoNotShowAgainButton->id = WZ_NOTIFY_DONOTSHOWAGAINCB_ID;
				pDoNotShowAgainButton->pText = _("Do not show again");
				pDoNotShowAgainButton->FontID = font_small;
				Vector2i minimumDimensions = pDoNotShowAgainButton->calculateDesiredDimensions();
				pDoNotShowAgainButton->setGeometry(WZ_NOTIFICATION_PADDING, buttonsTop, minimumDimensions.x, std::max(minimumDimensions.y, WZ_NOTIFICATION_BUTTON_HEIGHT));
	//			pDoNotShowAgainButton->setOnCheckStateChanged([](WzCheckboxButton& button, bool isChecked) {
	//
	//			});
				psNewNotificationForm->pOnDoNotShowAgainCheckbox = pDoNotShowAgainButton;
			}
		}

		// calculate the required height for the notification
		int bottom_labelContents = label_contents->y() + label_contents->height();
		calculatedHeight = bottom_labelContents + WZ_NOTIFICATION_PADDING;
		if (psActionButton || psDismissButton)
		{
			int maxButtonBottom = std::max<int>((psActionButton) ? (psActionButton->y() + psActionButton->height()) : 0, (psDismissButton) ? (psDismissButton->y() + psDismissButton->height()) : 0);
			calculatedHeight = std::max<int>(calculatedHeight, maxButtonBottom + WZ_NOTIFICATION_PADDING);
		}
		// Also factor in the image, if one is present
		if (imageSize > 0)
		{
			calculatedHeight = std::max<int>(calculatedHeight, imageSize + (WZ_NOTIFICATION_PADDING * 2));
		}
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

// run in-game notifications queue
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

void finishedProcessingNotificationRequest(WZ_Queued_Notification* request, bool doNotShowAgain)
{
	// at the moment, we only support processing a single notification at a time

	if (doNotShowAgain)
	{
		ASSERT(!request->notification.displayOptions.uniqueNotificationIdentifier().empty(), "Do Not Show Again was selected, but notification has no ignore key");
		debug(LOG_ERROR, "Do Not Show Notification Again: %s", request->notification.displayOptions.uniqueNotificationIdentifier().c_str());
		notificationPrefs->doNotShowNotificationAgain(request->notification.displayOptions.uniqueNotificationIdentifier());
	}

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
