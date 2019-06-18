//
// VkhInfo
// Version: 1.0
//
// Copyright (c) 2019 past-due
//
// https://github.com/past-due/vulkan-helpers
//
// Distributed under the MIT License.
// See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT
//

#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include "vkh_info.hpp"

#include <sstream>

VkhInfo::VkhInfo(const outputHandlerFuncType& _outputHandler)
: outputHandler(_outputHandler)
{ }

void VkhInfo::setOutputHandler(const outputHandlerFuncType& _outputHandler)
{
	outputHandler = _outputHandler;
}

bool VkhInfo::getInstanceExtensions(std::vector<VkExtensionProperties> &output, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	if (!_vkGetInstanceProcAddr) return false;

	PFN_vkEnumerateInstanceExtensionProperties _vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(reinterpret_cast<void*>(_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties")));
	if (!_vkEnumerateInstanceExtensionProperties)
	{
		// Could not find symbol: vkEnumerateInstanceExtensionProperties
		return false;
	}
	uint32_t extensionCount;
	_vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // get number of extensions

	std::vector<VkExtensionProperties> _extensions(extensionCount);
	_vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, _extensions.data()); // populate buffer

	output = _extensions;
	return true;
}

bool VkhInfo::supportsInstanceExtension(const char * extensionName, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	std::vector<VkExtensionProperties> extensions;
	if (!VkhInfo::getInstanceExtensions(extensions, _vkGetInstanceProcAddr))
	{
		return false;
	}

	return std::find_if(extensions.begin(), extensions.end(),
						[extensionName](const VkExtensionProperties& props) {
							return (strcmp(props.extensionName, extensionName) == 0);
						}) != extensions.end();
}

void VkhInfo::Output_InstanceExtensions(PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr)
{
	std::stringstream buf;

	// Supported instance extensions
	std::vector<VkExtensionProperties> supportedInstanceExtensions;
	if (VkhInfo::getInstanceExtensions(supportedInstanceExtensions, _vkGetInstanceProcAddr))
	{
		buf << "Instance Extensions:\n";
		buf << "====================\n";
		buf << "Count: " << supportedInstanceExtensions.size() << "\n";
		for (auto & extension : supportedInstanceExtensions)
		{
			buf << "- " << extension.extensionName << " (version: " << extension.specVersion << ")\n";
		}
		buf << "\n";
	}
	else
	{
		// Failure to request supported extensions
		buf << "Failed to retrieve supported instance extensions\n";
	}

	if (outputHandler)
	{
		outputHandler(buf.str());
	}
}
