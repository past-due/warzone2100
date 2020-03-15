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

	------

	Portions of this file are derived from cURL examples, which are licensed under the curl license.
	(The terms are also available at https://curl.haxx.se/docs/copyright.html.)

	curl license:
	COPYRIGHT AND PERMISSION NOTICE

	Copyright (c) 1996 - 2020, Daniel Stenberg, daniel@haxx.se, and many
	contributors, see the THANKS file.

	All rights reserved.

	Permission to use, copy, modify, and distribute this software for any purpose
	with or without fee is hereby granted, provided that the above copyright notice
	and this permission notice appear in all copies.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
	DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	Except as contained in this notice, the name of a copyright holder shall not be
	used in advertising or otherwise to promote the sale, use or other dealings in
	this Software without prior written authorization of the copyright holder.
*/

#define NOMINMAX
#include "urlrequest.h"
#include "lib/framework/wzapp.h"
#include <atomic>
#include <list>
#include <algorithm>
#include <stdio.h>
#include <vector>
#include <regex>

#undef max
#undef min

static volatile bool urlRequestQuit = false;

static WZ_THREAD        *urlRequestThread = nullptr;
static WZ_MUTEX         *urlRequestMutex = nullptr;
static WZ_SEMAPHORE     *urlRequestSemaphore = nullptr;

// MARK: - Handle thread-safety for cURL

#if defined(USE_OPENSSL_LOCKS_INIT)
# include <openssl/crypto.h>

// lock callbacks are only needed for OpenSSL < 1.1.0
# if OPENSSL_VERSION_NUMBER < 0x10100000L

/* we have this global to let the callback get easy access to it */
static std::vector<WZ_MUTEX *> lockarray;

static void lock_callback(int mode, int type, char *file, int line)
{
	(void)file;
	(void)line;
	if(mode & CRYPTO_LOCK) {
		wzMutexLock(lockarray[type]);
	}
	else {
		wzMutexUnlock(lockarray[type]);
	}
}

#  if OPENSSL_VERSION_NUMBER >= 0x10000000
// for CRYPTO_THREADID_set_callback API
static void thread_id(CRYPTO_THREADID *id)
{
	CRYPTO_THREADID_set_numeric(id, (unsigned long)wzThreadID(nullptr));
}
#  else
// for old CRYPTO_set_id_callback API
static unsigned long thread_id(void)
{
  unsigned long ret;

  ret = (unsigned long)wzThreadID(nullptr);
  return ret;
}
#  endif

static void init_locks(void)
{
	lockarray.resize(CRYPTO_num_locks());

	for(size_t i = 0; i < lockarray.size(); i++) {
		lockarray[i] = wzMutexCreate();
	}
#  if OPENSSL_VERSION_NUMBER >= 0x10000000
	CRYPTO_THREADID_set_callback(thread_id);
#  else
	CRYPTO_set_id_callback(thread_id);
#  endif
	CRYPTO_set_locking_callback(lock_callback);
}

static void kill_locks(void)
{
	CRYPTO_set_locking_callback(NULL);
	for(size_t i = 0; i < lockarray.size(); i++) {
		wzMutexDestroy(lockarray[i]);
	}

	lockarray.clear();
}
# else // no OpenSSL lock callbacks needed
# define init_locks()
# define kill_locks()
# endif
#elif defined(USE_OLD_GNUTLS_LOCKS_INIT)
# include <gcrypt.h>
# include <errno.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;

void init_locks(void)
{
  gcry_control(GCRYCTL_SET_THREAD_CBS);
}

# define kill_locks()
#else
# define init_locks()
# define kill_locks()
#endif

struct SemVer
{
	unsigned long major;
	unsigned long minor;
	unsigned long patch;

	SemVer()
	: major(0)
	, minor(0)
	, patch(0)
	{ }

	SemVer(unsigned long major, unsigned long minor, unsigned long patch)
	: major(major)
	, minor(minor)
	, patch(patch)
	{ }

	bool operator <(const SemVer& b) {
		return (major < b.major) ||
				((major == b.major) && (
					(minor < b.minor) ||
					((minor == b.minor) && (patch < b.patch))
				));
//		if(major < b.major) {
//			return true;
//		}
//		if(major == b.major) {
//			if(minor < b.minor) {
//				return true;
//			}
//			if(minor == b.minor) {
//				return patch < b.patch;
//			}
//			return false;
//		}
//		return false;
	}
	bool operator <=(const SemVer& b) {
		return (major < b.major) ||
				((major == b.major) && (
					(minor < b.minor) ||
					((minor == b.minor) && (patch <= b.patch))
				));
//		if(major < b.major) {
//			return true;
//		}
//		if(major == b.major) {
//			if(minor < b.minor) {
//				return true;
//			}
//			if(minor == b.minor) {
//				return patch <= b.patch;
//			}
//			return false;
//		}
//		return false;
	}
	bool operator >(const SemVer& b) {
		return !(*this <= b);
	}
	bool operator >=(const SemVer& b) {
		return !(*this < b);
	}
};

bool verify_curl_ssl_thread_safe_setup()
{
	// verify SSL backend version (if possible explicit thread-safety locks setup is required)
	const curl_version_info_data * info = curl_version_info(CURLVERSION_NOW);
	std::cmatch cm;

	const char* ssl_version_str = info->ssl_version;

	// GnuTLS
	std::regex gnutls_regex("^GnuTLS\\/([\\d]+)\\.([\\d]+)\\.([\\d]+).*");
	std::regex_match(ssl_version_str, cm, gnutls_regex);
	if(!cm.empty())
	{
		SemVer version;
		try {
			version.major = std::stoul(cm[1]);
			version.minor = std::stoul(cm[2]);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string to unsigned long because of error: %s", e.what());
		}

		// explicit gcry_control() is required when GnuTLS < 2.11.0
		SemVer min_safe_version = {2, 11, 0};
		if (version >= min_safe_version)
		{
			return true;
		}

		// explicit gcry_control() is required when GnuTLS < 2.11.0
#if defined(USE_OPENSSL_LOCKS_INIT) || !defined(USE_OLD_GNUTLS_LOCKS_INIT)
		// but we didn't initialize it
		return false;
#else
		// and we *did* initialize it
		return true;
#endif
	}

	// OpenSSL, libressl, BoringSSL
	std::regex e("^(OpenSSL|libressl|BoringSSL)\\/([\\d]+)\\.([\\d]+)\\.([\\d]+).*");
	std::regex_match(ssl_version_str, cm, e);
	if(!cm.empty())
	{
		ASSERT(cm.size() == 5, "Unexpected # of match results: %zu", cm.size());
		std::string variant = cm[1];
		SemVer version;
		try {
			version.major = std::stoul(cm[2]);
			version.minor = std::stoul(cm[3]);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string to unsigned long because of error: %s", e.what());
		}
		if (variant == "OpenSSL")
		{
			// for OpenSSL < 1.1.0, callbacks must be set
			SemVer min_safe_version = {1, 1, 0};
			if (version >= min_safe_version)
			{
				return true;
			}
			// for OpenSSL < 1.1.0, callbacks must be set
#if !defined(USE_OPENSSL_LOCKS_INIT)
			// but we didn't set them
			return false;
#else
			// and we *did* set them
			return true;
#endif
		}
		else
		{
			// TODO: Handle libressl, BoringSSL
		}
	}

	// otherwise, ssl backend should be thread-safe automatically
	return true;
}

// MARK: - Handle progress callbacks

#if LIBCURL_VERSION_NUM >= 0x073d00
/* In libcurl 7.61.0, support was added for extracting the time in plain
   microseconds. Older libcurl versions are stuck in using 'double' for this
   information so we complicate this example a bit by supporting either
   approach. */
#define TIME_IN_US 1
#define TIMETYPE curl_off_t
#define TIMEOPT CURLINFO_TOTAL_TIME_T
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3000000
#else
#define TIMETYPE double
#define TIMEOPT CURLINFO_TOTAL_TIME
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#endif

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MAXIMUM_DOWNLOAD_SIZE                       2147483647L

class URLTransferRequest;

struct myprogress {
  TIMETYPE lastruntime = (TIMETYPE)0; /* type depends on version, see above */
  URLTransferRequest *request;
};

// MARK: -

static size_t WriteMemoryCallback_URLTransferRequest(void *contents, size_t size, size_t nmemb, void *userp);
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow);
#endif

class URLTransferRequest
{
public:
	CURL *handle = nullptr;
	struct myprogress progress;

public:
	virtual ~URLTransferRequest() {}

	virtual const std::string& url() const = 0;
	virtual curl_off_t maxDownloadSize() const { return MAXIMUM_DOWNLOAD_SIZE; }

	virtual CURL* createCURLHandle()
	{
		// Create cURL easy handle
		handle = curl_easy_init();
		if (!handle)
		{
			// Something went wrong with curl_easy_init
			return nullptr;
		}
		curl_easy_setopt(handle, CURLOPT_URL, url().c_str());
		if (hasWriteMemoryCallback())
		{
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback_URLTransferRequest);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)this);
		}
	#if LIBCURL_VERSION_NUM >= 0x071304	// cURL 7.19.4+
		/* only allow HTTP and HTTPS */
		curl_easy_setopt(handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	#endif
		/* tell libcurl to follow redirection */
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
		/* set max redirects */
		curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L);

		progress.request = this;
	#if LIBCURL_VERSION_NUM >= 0x072000
		/* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
		   compile as they won't have the symbols around.

		   If built with a newer libcurl, but running with an older libcurl:
		   curl_easy_setopt() will fail in run-time trying to set the new
		   callback, making the older callback get used.

		   New libcurls will prefer the new callback and instead use that one even
		   if both callbacks are set. */

		curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
		/* pass the struct pointer into the xferinfo function, note that this is
		   an alias to CURLOPT_PROGRESSDATA */
		curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &progress);
	#else
		curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, older_progress);
		/* pass the struct pointer into the progress function */
		curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &progress);
	#endif

		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);

	#if LIBCURL_VERSION_NUM >= 0x070B00	// cURL 7.11.0+
		/* refuse to download if larger than limit */
		curl_off_t downloadSizeLimit = maxDownloadSize();
		if (downloadSizeLimit > 0)
		{
			curl_easy_setopt(handle, CURLOPT_MAXFILESIZE_LARGE, downloadSizeLimit);
		}
	#endif

	#if LIBCURL_VERSION_NUM >= 0x070A00	// cURL 7.10.0+
		curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
	#endif

		/* abort if slower than 30 bytes/sec during 60 seconds */
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 60L);
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 30L);

		return handle;
	}

	virtual bool hasWriteMemoryCallback() { return false; }
	virtual size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb) { return 0; }

	virtual bool onProgressUpdate(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) { return false; }

	virtual bool waitOnShutdown() const { return false; }

	virtual void handleRequestDone(CURLcode result) { }
};

static size_t
WriteMemoryCallback_URLTransferRequest(void *contents, size_t size, size_t nmemb, void *userp)
{
	// expects that userp will be the URLTransferRequest
	if (!userp)
	{
		return 0;
	}

	URLTransferRequest* pRequest = static_cast<URLTransferRequest*>(userp);
	return pRequest->writeMemoryCallback(contents, size, nmemb);
}

/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
	int retValue = 0;
	struct myprogress *myp = (struct myprogress *)p;
	CURL *curl = myp->request->handle;
	TIMETYPE curtime = 0;

	curl_easy_getinfo(curl, TIMEOPT, &curtime);

	/* under certain circumstances it may be desirable for certain functionality
	 to only run every N seconds, in order to do this the transaction time can
	 be used */
	if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
		myp->lastruntime = curtime;
		#ifdef TIME_IN_US
		fprintf(stderr, "TOTAL TIME: %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n",
				(curtime / 1000000), (long)(curtime % 1000000));
		#else
		fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
		#endif

		retValue = myp->request->onProgressUpdate(dltotal, dlnow, ultotal, ulnow);
	}

//	fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
//		  "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
//		  "\r\n",
//		  ulnow, ultotal, dlnow, dltotal);

//	if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
//		return 1;
	// prevent infinite download denial-of-service
	if(dlnow > myp->request->maxDownloadSize())
		return 1;
	return retValue;
}

#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
  return xferinfo(p,
                  (curl_off_t)dltotal,
                  (curl_off_t)dlnow,
                  (curl_off_t)ultotal,
                  (curl_off_t)ulnow);
}
#endif

class RunningURLDataRequest : public URLTransferRequest
{
public:
	URLDataRequest request;
	std::shared_ptr<MemoryStruct> chunk;
private:
	curl_off_t _maxDownloadSize = 1 << 29; // 512 MB, default max download limit

public:
	RunningURLDataRequest(const URLDataRequest& request)
	: request(request)
	{
		chunk = std::make_shared<MemoryStruct>();
		if (request.maxDownloadSizeLimit > 0)
		{
			_maxDownloadSize = std::min(request.maxDownloadSizeLimit, _maxDownloadSize);
		}
	}

	virtual const std::string& url() const override
	{
		return request.url;
	}

	virtual curl_off_t maxDownloadSize() const override
	{
		// For downloading to memory, set a lower default max download limit
		return _maxDownloadSize;
	}

	virtual bool hasWriteMemoryCallback() override { return true; }
	virtual size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb) override
	{
		size_t realsize = size * nmemb;
		MemoryStruct *mem = chunk.get();

		char *ptr = (char*) realloc(mem->memory, mem->size + realsize + 1);
		if(ptr == NULL) {
			/* out of memory! */
			printf("not enough memory (realloc returned NULL)\n");
			return 0;
		}

		mem->memory = ptr;
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;

		return realsize;
	}

	virtual bool onProgressUpdate(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) override
	{
		fprintf(stderr, "UP: %" PRId64 " of %" PRId64 "  DOWN: %" PRId64 " of %" PRId64 "\r\n",
				ulnow, ultotal, dlnow, dltotal);
		if (request.progressCallback)
		{
			request.progressCallback(request.url, dltotal, dlnow);
		}
		return false;
	}

	virtual void handleRequestDone(CURLcode result) override
	{
		long code;
		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);

		if (result == CURLE_OK)
		{
			onSuccess();
		}
		else
		{
			onFailure(result, code);
		}
	}

private:
	void onSuccess()
	{
		if (request.onSuccess)
		{
			request.onSuccess(request.url, chunk);
		}
	}

	void onFailure(CURLcode result, long httpResponseCode)
	{
		if (request.onFailure)
		{
			request.onFailure(request.url, result, httpResponseCode);
		}
		else
		{
			const std::string& url = request.url;
			wzAsyncExecOnMainThread([url, result, httpResponseCode]{
				debug(LOG_ERROR, "Request for (%s) failed with error %d, and HTTP response code: %ld", url.c_str(), result, httpResponseCode);
			});
		}
	}
};

class RunningURLFileDownloadRequest : public URLTransferRequest
{
public:
	URLFileDownloadRequest request;
	FILE *outFile;

	RunningURLFileDownloadRequest(const URLFileDownloadRequest& request)
	: request(request)
	{
#if defined(WZ_OS_WIN)
		// On Windows, path strings passed to fopen() are interpreted using the ANSI or OEM codepage
		// (and not as UTF-8). To support Unicode paths, the string must be converted to a wide-char
		// string and passed to _wfopen.
		int wstr_len = MultiByteToWideChar(CP_UTF8, 0, request.outFilePath.c_str(), -1, NULL, 0);
		if (wstr_len <= 0)
		{
			fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar failed with error %d: %s\n", GetLastError(), request.outFilePath.c_str());
			// TODO: throw exception?
			return;
		}
		std::vector<wchar_t> wstr_filename(wstr_len, 0);
		if (MultiByteToWideChar(CP_UTF8, 0, request.outFilePath.c_str(), -1, &wstr_filename[0], wstr_len) == 0)
		{
			fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error %d: %s\n", GetLastError(), request.outFilePath.c_str());
			// TODO: throw exception?
			return;
		}
		outFile = _wfopen(&wstr_filename[0], L"wb");
#else
		outFile = fopen(request.outFilePath.c_str(), "wb");
#endif
		if (!outFile)
		{
			fprintf(stderr, "Could not open %s for output!\n", request.outFilePath.c_str());
			// TODO: throw exception?
		}
	}

	virtual const std::string& url() const override
	{
		return request.url;
	}

	virtual bool hasWriteMemoryCallback() override { return true; }
	virtual size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb) override
	{
		if (!outFile) return 0;
		size_t written = fwrite(contents, size, nmemb, outFile);
		return written;
	}

	virtual bool onProgressUpdate(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) override
	{
		fprintf(stderr, "UP: %" PRId64 " of %" PRId64 "  DOWN: %" PRId64 " of %" PRId64 "\r\n",
				ulnow, ultotal, dlnow, dltotal);
		if (request.progressCallback)
		{
			request.progressCallback(request.url, dltotal, dlnow);
		}
		return false;
	}

	virtual void handleRequestDone(CURLcode result) override
	{
		if (outFile)
		{
			fclose(outFile);
		}

		long code;
		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);

		if (result == CURLE_OK)
		{
			onSuccess();
		}
		else
		{
			onFailure(result, code);
		}
	}

private:
	void onSuccess()
	{
		if (request.onSuccess)
		{
			request.onSuccess(request.url, request.outFilePath);
		}
	}

	void onFailure(CURLcode result, long httpResponseCode)
	{
		if (request.onFailure)
		{
			request.onFailure(request.url, result, httpResponseCode);
		}
		else
		{
			const std::string& url = request.url;
			wzAsyncExecOnMainThread([url, result, httpResponseCode]{
				debug(LOG_ERROR, "Request for (%s) failed with error %d, and HTTP response code: %ld", url.c_str(), result, httpResponseCode);
			});
		}
	}
};

static std::list<URLTransferRequest*> newUrlRequests;

/** This runs in a separate thread */
static int urlRequestThreadFunc(void *)
{
	// initialize cURL
	CURLM *multi_handle = curl_multi_init();

	wzMutexLock(urlRequestMutex);

	std::list<URLTransferRequest*> runningTransfers;
	int transfers_running = 0;
	CURLMcode multiCode = CURLM_OK;
	while (!urlRequestQuit)
	{
		if (newUrlRequests.empty() && transfers_running == 0)
		{
			wzMutexUnlock(urlRequestMutex);
			wzSemaphoreWait(urlRequestSemaphore);  // Go to sleep until needed.
			wzMutexLock(urlRequestMutex);
			continue;
		}

		// Start handling new requests
		while(!newUrlRequests.empty())
		{
			URLTransferRequest* newRequest = newUrlRequests.front();
			newUrlRequests.pop_front();

			// Create cURL easy handle
			if (!newRequest->createCURLHandle())
			{
				// something went wrong creating the handle
				// TODO: log
				delete newRequest;
				continue; // skip to next request
			}

			// Add to multi-handle
			multiCode = curl_multi_add_handle(multi_handle, newRequest->handle);
			if (multiCode != CURLM_OK)
			{
				// curl_multi_add_handle failed
				wzAsyncExecOnMainThread([multiCode]{
					debug(LOG_ERROR, "curl_multi_add_handle failed: %d", multiCode);
				});
				// TODO: call failure handler
				curl_easy_cleanup(newRequest->handle);
				delete newRequest;
				continue; // skip to next request
			}
			runningTransfers.push_back(newRequest);
		}
		newUrlRequests.clear();

		wzMutexUnlock(urlRequestMutex); // when performing / waiting on curl requests, unlock the mutex

		multiCode = curl_multi_perform ( multi_handle, &transfers_running );
		if (multiCode == CURLM_OK )
		{
			/* wait for activity, timeout or "nothing" */
		#if LIBCURL_VERSION_NUM >= 0x071C00	// cURL 7.28.0+ required for curl_multi_wait
			multiCode = curl_multi_wait ( multi_handle, NULL, 0, 1000, NULL);
		#else
			#error "Needs a fallback for lack of curl_multi_wait (or, even better, update libcurl!)"
		#endif
		}
		if (multiCode != CURLM_OK)
		{
			fprintf(stderr, "curl_multi failed, code %d.\n", multiCode);
			break;
		}

		// Handle finished transfers
		CURLMsg *msg; /* for picking up messages with the transfer status */
		int msgs_left; /* how many messages are left */
		while((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
			if(msg->msg == CURLMSG_DONE) {
				CURL *e = msg->easy_handle;
				CURLcode result = msg->data.result;

				printf("HTTP transfer completed with status %d\n", msg->data.result);

				/* Find out which handle this message is about */
				auto it = std::find_if(runningTransfers.begin(), runningTransfers.end(), [e](const URLTransferRequest* req) {
					return req->handle == e;
				});

				if (it != runningTransfers.end())
				{
					URLTransferRequest* urlTransfer = *it;
					urlTransfer->handleRequestDone(result);
					delete urlTransfer;
					runningTransfers.erase(it);
				}
				else
				{
					// log failure to find request in running transfers list
					wzAsyncExecOnMainThread([]{
						debug(LOG_ERROR, "Failed to find request in running transfers list");
					});
				}

				curl_multi_remove_handle(multi_handle, e);
				curl_easy_cleanup(e);
			}
		}

		wzMutexLock(urlRequestMutex);
	}
	wzMutexUnlock(urlRequestMutex);

	auto it = runningTransfers.begin();
	while (it != runningTransfers.end())
	{
		URLTransferRequest* runningTransfer = *it;
		if (!runningTransfer->waitOnShutdown())
		{
			curl_multi_remove_handle(multi_handle, runningTransfer->handle);
			curl_easy_cleanup(runningTransfer->handle);
			delete runningTransfer;
			it = runningTransfers.erase(it);
		}
		else
		{
			++it;
		}
	}

	// TODO: Wait for all remaining ("waitOnShutdown") transfers to complete

	curl_multi_cleanup(multi_handle);
	return 0;
}

void urlRequestData(const URLDataRequest& request)
{
	wzMutexLock(urlRequestMutex);
	bool isFirstJob = newUrlRequests.empty();
	newUrlRequests.push_back(new RunningURLDataRequest(request));
	wzMutexUnlock(urlRequestMutex);

	if (isFirstJob)
	{
		wzSemaphorePost(urlRequestSemaphore);  // Wake up processing thread.
	}
}

void urlDownloadFile(const URLFileDownloadRequest& request)
{
	wzMutexLock(urlRequestMutex);
	bool isFirstJob = newUrlRequests.empty();
	newUrlRequests.push_back(new RunningURLFileDownloadRequest(request));
	wzMutexUnlock(urlRequestMutex);

	if (isFirstJob)
	{
		wzSemaphorePost(urlRequestSemaphore);  // Wake up processing thread.
	}
}

#if LIBCURL_VERSION_NUM >= 0x073800 // cURL 7.56.0+
std::vector<std::pair<std::string, curl_sslbackend>> listSSLBackends()
{
	// List available cURL SSL backends
	std::vector<std::pair<std::string, curl_sslbackend>> output;

	const curl_ssl_backend **list;
	int i;
	CURLsslset result = curl_global_sslset((curl_sslbackend)-1, NULL, &list);
	if(result != CURLSSLSET_UNKNOWN_BACKEND)
	{
		return output;
	}
	for(i = 0; list[i]; i++)
	{
		output.push_back({list[i]->name, list[i]->id});
	}
	return output;
}
#endif

void urlRequestOutputDebugInfo()
{
	const curl_version_info_data * info = curl_version_info(CURLVERSION_NOW);
	if (info->age >= 0)
	{
		debug(LOG_WZ, "cURL: %s", info->version);
		debug(LOG_WZ, "- SSL: %s", info->ssl_version);

		#if LIBCURL_VERSION_NUM >= 0x073800 // cURL 7.56.0+
		if (!!(info->features & CURL_VERSION_MULTI_SSL))
		{
			auto availableSSLBackends = listSSLBackends();
			std::string sslBackendStr;
			for (const auto &backend : availableSSLBackends)
			{
				if (!sslBackendStr.empty())
				{
					sslBackendStr += ",";
				}
				sslBackendStr += backend.first;
			}
			debug(LOG_WZ, "- Available SSL backends: %s", sslBackendStr.c_str());
		}
		#endif
		#if LIBCURL_VERSION_NUM >= 0x070A07 // cURL 7.10.7+
		debug(LOG_WZ, "- AsynchDNS: %d", !!(info->features & CURL_VERSION_ASYNCHDNS));
		#endif
	}

	if (info->age >= 1)
	{
		if (info->ares)
		{
			debug(LOG_WZ, "- ares: %s", info->ares);
		}
	}
	if (info->age >= 2)
	{
		if (info->libidn)
		{
			debug(LOG_WZ, "- libidn: %s", info->libidn);
		}
	}
}

void urlRequestInit()
{
#if LIBCURL_VERSION_NUM >= 0x073800 // cURL 7.56.0+
	auto availableSSLBackends = listSSLBackends();
	// Note: Use CURLSSLBACKEND_DARWINSSL instead of CURLSSLBACKEND_SECURETRANSPORT to support older cURL versions
	const std::vector<curl_sslbackend> backendPreferencesOrder = {CURLSSLBACKEND_SCHANNEL, CURLSSLBACKEND_DARWINSSL, CURLSSLBACKEND_GNUTLS, CURLSSLBACKEND_NSS};
	std::vector<curl_sslbackend> ignoredBackends;
#if !defined(USE_OPENSSL_LOCKS_INIT) && !defined(CURL_OPENSSL_DOES_NOT_REQUIRE_LOCKS_INIT)
	// Did not compile with support for thread-safety / locks for OpenSSL, so ignore it
	ignoredBackends.push_back(CURLSSLBACKEND_OPENSSL);
#endif
#if !defined(USE_OLD_GNUTLS_LOCKS_INIT) && !defined(CURL_GNUTLS_DOES_NOT_REQUIRE_LOCKS_INIT)
	// Did not compile with support for thread-safety / locks for GnuTLS, so ignore it
	ignoredBackends.push_back(CURLSSLBACKEND_GNUTLS);
#endif
	if (!ignoredBackends.empty())
	{
		availableSSLBackends.erase(std::remove_if(availableSSLBackends.begin(),
					   availableSSLBackends.end(),
					   [ignoredBackends](const std::pair<std::string, curl_sslbackend>& backend)
		{
			return std::find(ignoredBackends.begin(), ignoredBackends.end(), backend.second) != ignoredBackends.end();
		}),
		availableSSLBackends.end());
	}
	if (!availableSSLBackends.empty())
	{
		curl_sslbackend sslBackendChoice = availableSSLBackends.front().second;
		for (const auto& preferredBackend : backendPreferencesOrder)
		{
			auto it = std::find_if(availableSSLBackends.begin(), availableSSLBackends.end(), [preferredBackend](const std::pair<std::string, curl_sslbackend>& backend) {
				return backend.second == preferredBackend;
			});
			if (it != availableSSLBackends.end())
			{
				sslBackendChoice = it->second;
				break;
			}
		}
		if (sslBackendChoice != CURLSSLBACKEND_NONE)
		{
			CURLsslset result = curl_global_sslset(sslBackendChoice, nullptr, nullptr);
			if (result == CURLSSLSET_OK)
			{
				debug(LOG_WZ, "cURL: selected SSL backend: %d", sslBackendChoice);
			}
			else
			{
				debug(LOG_WZ, "cURL: failed to select SSL backend (%d) with error: %d", sslBackendChoice, result);
			}
		}
	}
	else
	{
		debug(LOG_ERROR, "cURL has no available thread-safe SSL backends to configure");
		for (const auto& ignoredBackend : ignoredBackends)
		{
			debug(LOG_ERROR, "(Ignored backend: %d (build did not permit thread-safe configuration)", ignoredBackend);
		}
	}
#endif

	/* Must initialize libcurl before any threads are started */
	CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
	if (result != CURLE_OK)
	{
		debug(LOG_ERROR, "curl_global_init failed: %d", result);
	}

	init_locks();

	if (!verify_curl_ssl_thread_safe_setup())
	{
		debug(LOG_ERROR, "curl initialized, but ssl backend could not be configured to be thread-safe");
	}

	// start background thread for processing HTTP requests
	urlRequestQuit = false;

	if (!urlRequestThread)
	{
		urlRequestMutex = wzMutexCreate();
		urlRequestSemaphore = wzSemaphoreCreate(0);
		urlRequestThread = wzThreadCreate(urlRequestThreadFunc, nullptr);
		wzThreadStart(urlRequestThread);
	}
}

void urlRequestShutdown()
{
	if (urlRequestThread)
	{
		// Signal the path finding thread to quit
		urlRequestQuit = true;
		wzSemaphorePost(urlRequestSemaphore);  // Wake up thread.

		wzThreadJoin(urlRequestThread);
		urlRequestThread = nullptr;
		wzMutexDestroy(urlRequestMutex);
		urlRequestMutex = nullptr;
		wzSemaphoreDestroy(urlRequestSemaphore);
		urlRequestSemaphore = nullptr;
	}

	kill_locks();

	curl_global_cleanup();
}
