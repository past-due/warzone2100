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
#ifndef _WEB_REQUESTER_H_
#define _WEB_REQUESTER_H_

#include <functional>
#include <curl/curl.h>
#include <string>
#include <memory>

struct MemoryStruct {
	char *memory = nullptr;
	size_t size = 0;

	MemoryStruct()
	{
		memory = (char *) malloc(1);
		size = 0;
	}

	~MemoryStruct()
	{
		if (memory != nullptr)
		{
			free(memory);
			memory = nullptr;
		}
	}
};

typedef std::function<void (const std::string& url, const std::shared_ptr<MemoryStruct>& data)> UrlRequestSuccess;
typedef std::function<void (const std::string& url, CURLcode result, long httpResponseCode)> UrlRequestFailure;
typedef std::function<void (const std::string& url, int64_t dltotal, int64_t dlnow)> UrlProgressCallback;

struct URLDataRequest
{
	std::string url;
	curl_off_t maxDownloadSizeLimit = 0;

	// callbacks
	UrlProgressCallback progressCallback;
	UrlRequestSuccess onSuccess;
	UrlRequestFailure onFailure;
};

typedef std::function<void (const std::string& url, const std::string& outFilePath)> UrlDownloadFileSuccess;

struct URLFileDownloadRequest
{
	std::string url;
	std::string outFilePath;

	// callbacks
	UrlProgressCallback progressCallback;
	UrlDownloadFileSuccess onSuccess;
	UrlRequestFailure onFailure;
};

void urlRequestData(const URLDataRequest& request);
void urlDownloadFile(const URLFileDownloadRequest& request);

void urlRequestInit();
void urlRequestOutputDebugInfo();
void urlRequestShutdown();

#endif //_WEB_REQUESTER_H_
