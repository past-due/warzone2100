/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#include "updatemanager.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzstring.h"
#include "urlhelpers.h"
#include "urlrequest.h"
#include "notifications.h"
#include <EmbeddedJSONSignature.h>
#include "propertymatcher.h"
#include "wzpropertyproviders.h"

#include <sodium.h>
#include <re2/re2.h>

#include <3rdparty/json/json.hpp>
using json = nlohmann::json;

class WzUpdateManager {
public:
	static void initUpdateCheck();
private:
	static void processUpdateJSONFile(const json& updateData, bool validSignature);
	static std::string configureUpdateLinkURL(const std::string& url, BuildPropertyProvider& propProvider);
};

const std::string WZ_UPDATES_VERIFY_KEY = "5d9P+Z1SirsWSsYICZAr7QFlPB01s6tzXkhPZ+X/FQ4=";
const std::string WZ_DEFAULT_UPDATE_LINK = "https://warzone2100.github.io/update-data/redirect/updatelink.html";

// Replaces specific build property keys with their values in a URL string
// Build property keys are surrounded by "{{}}" - i.e. "{{PLATFORM}}" is replaced with the value of the PLATFORM build property
// May be called from a background thread
std::string WzUpdateManager::configureUpdateLinkURL(const std::string& url, BuildPropertyProvider& propProvider)
{
	const std::unordered_set<std::string> permittedBuildPropertySubstitutions = { "PLATFORM", "VERSION_STRING", "GIT_BRANCH" };

	std::vector<std::string> tokens;
	re2::StringPiece input(url);
	std::string token;

	RE2 re("({{[\\S]+}})");
	while (RE2::FindAndConsume(&input, re, &token))
	{
		tokens.push_back(token);
	}

	std::string resultUrl = url;
	for (const auto& token : tokens)
	{
		std::string::size_type pos = url.find(token, 0);
		if (pos != std::string::npos)
		{
			std::string propValue;
			std::string buildProperty = token.substr(2, token.length() - 4);
			if (permittedBuildPropertySubstitutions.count(buildProperty) > 0)
			{
				propProvider.getPropertyValue(buildProperty, propValue);
			}
			else
			{
				propValue = "prop_not_supported";
			}
			if (!propValue.empty())
			{
				propValue = urlEncode(propValue.c_str());
			}
			resultUrl.replace(pos, token.length(), propValue);
		}
	}

	return resultUrl;
}

// May be called from a background thread
void WzUpdateManager::processUpdateJSONFile(const json& updateData, bool validSignature)
{
	if (!updateData.is_object())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_ERROR, "Update data is not an object"); });
		return;
	}
	const auto& channels = updateData["channels"];
	if (!channels.is_array())
	{
		wzAsyncExecOnMainThread([]{ debug(LOG_ERROR, "Channels should be an array"); });
		return;
	}
	BuildPropertyProvider buildPropProvider;
	bool foundMatch = false;
	for (const auto& channel : channels)
	{
		if (!channel.is_object()) continue;
		const auto& channelName = channel["channel"];
		if (!channelName.is_string()) continue;
		std::string channelNameStr = channelName.get<std::string>();
		const auto& channelConditional = channel["channelConditional"];
		if (!channelConditional.is_string()) continue;
		if (!PropertyMatcher::evaluateConditionString(channelConditional.get<std::string>(), buildPropProvider))
		{
			// non-matching channel conditional
			continue;
		}
		const auto& releases = channel["releases"];
		if (!releases.is_array()) continue;
		for (const auto& release : releases)
		{
			const auto& buildPropertyMatch = release["buildPropertyMatch"];
			if (!buildPropertyMatch.is_string()) continue;
			if (!PropertyMatcher::evaluateConditionString(buildPropertyMatch.get<std::string>(), buildPropProvider))
			{
				// non-matching release buildPropertyMatch
				continue;
			}
			// it matches!
			// verify the release has the required properties
			const auto& releaseVersion = release["version"];
			if (!releaseVersion.is_string())
			{
				// release version is not a string
				continue;
			}
			json notificationInfo = release["notification"];
			if (!notificationInfo.is_object())
			{
				// TODO: Handle lack of notification info?
			}
			std::string updateLink;
			if (release.contains("updateLink"))
			{
				updateLink = release["updateLink"].get<std::string>();
			}
			bool hasValidURLPrefix = urlHasHTTPorHTTPSPrefix(updateLink.c_str());
			if (!validSignature || updateLink.empty() || !hasValidURLPrefix)
			{
				// use default update link
				updateLink = WZ_DEFAULT_UPDATE_LINK;
			}
			updateLink = configureUpdateLinkURL(updateLink, buildPropProvider);
			std::string releaseVersionStr = releaseVersion.get<std::string>();
			// submit notification (on main thread)
			wzAsyncExecOnMainThread([validSignature, channelNameStr, releaseVersionStr, notificationInfo, updateLink]{
				debug(LOG_ERROR, "Found an available update (%s) in channel (%s)", releaseVersionStr.c_str(), channelNameStr.c_str());
				WZ_Notification notification;
				notification.duration = 0;
				notification.contentTitle = _("Update Available");
				if (validSignature)
				{
					notification.contentText = astringf(_("A new build of Warzone 2100 (%s) is available!"), releaseVersionStr.c_str());
				}
				else
				{
					notification.contentText = _("A new build of Warzone 2100 is available!");
				}
				notification.action = WZ_Notification_Action(_("Get Update Now"), [updateLink](const WZ_Notification&){
					// Open the updateLink url
					wzAsyncExecOnMainThread([updateLink]{
						if (!openURLInBrowser(updateLink.c_str()))
						{
							debug(LOG_ERROR, "Failed to open url in browser: \"%s\"", updateLink.c_str());
						}
					});
				});
				notification.largeIconPath = "images/warzone2100.png";
				if (notificationInfo.is_object())
				{
					const auto& notificationBase = notificationInfo["base"];
					const auto& notificationId = notificationInfo["id"];
					if (notificationBase.is_string() && notificationId.is_string())
					{
						const std::string notificationIdentifierPrefix = notificationBase.get<std::string>() + "::";
						const std::string notificationIdentifier = notificationIdentifierPrefix + notificationId.get<std::string>();
						removeNotificationPreferencesIf([&notificationIdentifierPrefix, &notificationIdentifier](const std::string &uniqueNotificationIdentifier) -> bool {
							bool hasPrefix = (strncmp(uniqueNotificationIdentifier.c_str(), notificationIdentifierPrefix.c_str(), notificationIdentifierPrefix.size()) == 0);
							return hasPrefix && (notificationIdentifier != uniqueNotificationIdentifier);
						});
						notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable(notificationIdentifier, 3);
					}
				}
				addNotification(notification, WZ_Notification_Trigger::Immediate());
			});
			foundMatch = true;
			break;
		}
		if (foundMatch) break;
	}
}

void WzUpdateManager::initUpdateCheck()
{
	URLDataRequest request;
	request.url = "https://warzone2100.github.io/update-data/wz2100.json";
	request.onSuccess = [](const std::string& url, const std::shared_ptr<MemoryStruct>& data) {

		// Extract the digital signature, and verify it
		std::string updateJsonStr;
		bool validSignature = EmbeddedJSONSignature::verifySignedJson(data->memory, data->size, WZ_UPDATES_VERIFY_KEY, updateJsonStr);

		// Parse the remaining json (minus the digital signature)
		json updateData;
		try {
			updateData = json::parse(updateJsonStr);
		}
		catch (const std::exception &e) {
			wzAsyncExecOnMainThread([&e, &url]{
				debug(LOG_NET, "JSON document from %s is invalid: %s", url.c_str(), e.what());
			});
			return;
		}
		catch (...) {
			wzAsyncExecOnMainThread([&url]{
				debug(LOG_NET, "Unexpected exception parsing JSON %s", url.c_str());
			});
			return;
		}
		if (updateData.is_null())
		{
			wzAsyncExecOnMainThread([&url]{
				debug(LOG_NET, "JSON document from %s is null", url.c_str());
			});
			return ;
		}
		if (!updateData.is_object())
		{
			wzAsyncExecOnMainThread([&url, &data]{
				debug(LOG_NET, "JSON document from %s is not an object. Read: \n%s", url.c_str(), data->memory);
			});
			return;
		}

		// Process the updates JSON, notify if new version is available
		WzUpdateManager::processUpdateJSONFile(updateData, validSignature);
	};
	request.onFailure = [](const std::string& url, URLRequestFailureType type, optional<URLTransferFailedDetails> transferFailureDetails) {
		switch (type)
		{
			case URLRequestFailureType::INITIALIZE_REQUEST_ERROR:
				wzAsyncExecOnMainThread([]{
					debug(LOG_ERROR, "Failed to initialize request for update check");
				});
				break;
			case URLRequestFailureType::TRANSFER_FAILED:
				wzAsyncExecOnMainThread([transferFailureDetails]{
					if (!transferFailureDetails.has_value())
					{
						debug(LOG_ERROR, "Update check request failed - but no transfer failure details provided!");
						return;
					}
					debug(LOG_ERROR, "Update check request failed with error %d, and HTTP response code: %ld", transferFailureDetails.value().result, transferFailureDetails.value().httpResponseCode);
				});
				break;
			case URLRequestFailureType::CANCELLED_BY_SHUTDOWN:
				wzAsyncExecOnMainThread([url]{
					debug(LOG_ERROR, "Update check was cancelled by application shutdown");
				});
				break;
		}
	};
	request.maxDownloadSizeLimit = 1 << 25; // 32 MB (the response should never be this big)
	urlRequestData(request);
}

void WzInfoManager::initialize()
{
	WzUpdateManager::initUpdateCheck();
}

void WzInfoManager::shutdown()
{
	/* currently, no-op */
}
