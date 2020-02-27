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
 *  Interface to the initialisation routines.
 */

#ifndef __INCLUDED_SRC_NOTIFICATIONS_H__
#define __INCLUDED_SRC_NOTIFICATIONS_H__

#include <list>
#include <memory>

struct iV_Image;

class WZ_Notification
{
public:
	uint32_t duration = 0; // set to 0 for "until dismissed by user"
	std::string contentTitle;
	std::string contentText;
//	std::string smallIconPath;
	std::string largeIconPath;
	std::string actionButtonTitle;
	std::function<void (WZ_Notification&)> onClick;
	std::function<void (WZ_Notification&)> onDismissed;
	std::function<void (WZ_Notification&)> onDoNotShowAgain;
	iV_Image* largeIcon = nullptr;
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



bool notificationsInitialize();
void notificationsShutDown();

void runNotifications();
void addNotification(const WZ_Notification& notification, const WZ_Notification_Trigger& trigger);

bool isDraggingInGameNotification();
bool isMouseOverInGameNotification();

#endif // __INCLUDED_SRC_NOTIFICATIONS_H__
