
#if defined(__EMSCRIPTEN__)

#include "urlrequest.h"
#include "lib/framework/frame.h"

HTTPResponseHeaders::~HTTPResponseHeaders() { }
HTTPResponseDetails::~HTTPResponseDetails() { }
AsyncRequest::~AsyncRequest() { }

bool URLRequestBase::setRequestHeader(const std::string& name, const std::string& value)
{
	ASSERT_OR_RETURN(false, !name.empty(), "Header name must not be empty");
	ASSERT_OR_RETURN(false, name.find_first_of(":") == std::string::npos, "Header name must not contain ':'");
	requestHeaders[name] = value;
	return true;
}

// Request data from a URL (stores the response in memory)
// Generally, you should define both onSuccess and onFailure callbacks
// If you want to actually process the response, you *must* define an onSuccess callback
//
// IMPORTANT: Callbacks will be called on a background thread
AsyncURLRequestHandle urlRequestData(const URLDataRequest& request)
{
	// TODO: Implement
	return nullptr;
}

// Download a file (stores the response in the outFilePath)
// Generally, you should define both onSuccess and onFailure callbacks
//
// IMPORTANT: Callbacks will be called on a background thread
AsyncURLRequestHandle urlDownloadFile(const URLFileDownloadRequest& request)
{
	// TODO: Implement
	return nullptr;
}

// Sets a flag that will cancel an asynchronous url request
// NOTE: It is possible that the request will finish successfully before it is cancelled.
void urlRequestSetCancelFlag(AsyncURLRequestHandle requestHandle)\
{
	// TODO: Implement
}

void urlRequestInit()
{
	// TODO: Implement
}
void urlRequestOutputDebugInfo()
{
	// TODO: Implement
}
void urlRequestShutdown()
{
	// TODO: Implement
}

#endif
