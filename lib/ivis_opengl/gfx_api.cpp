/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2019  Warzone 2100 Project

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

#include "gfx_api_vk.h"
#include "gfx_api_gl.h"
#include "gfx_api_null.h"

static gfx_api::backend_type backend = gfx_api::backend_type::opengl_backend;
bool uses_gfx_debug = false;
static gfx_api::context* current_backend_context = nullptr;

bool gfx_api::context::initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode swapMode, gfx_api::backend_type backendType)
{
	if (current_backend_context != nullptr && backend == backendType)
	{
		// ignore re-init for same backendType (for now)
		debug(LOG_ERROR, "Attempt to re-initialize gfx_api::context for the same backend type - ignoring (for now)");
		return true;
	}
	backend = backendType;
	if (current_backend_context)
	{
		debug(LOG_FATAL, "Attempt to reinitialize gfx_api::context for a new backend type - currently unsupported");
		return false;
	}
	switch (backend)
	{
		case gfx_api::backend_type::null_backend:
			current_backend_context = new null_context(uses_gfx_debug);
			break;
		case gfx_api::backend_type::opengl_backend:
			current_backend_context = new gl_context(uses_gfx_debug);
			break;
		case gfx_api::backend_type::vulkan_backend:
#if defined(WZ_VULKAN_ENABLED)
			current_backend_context = new VkRoot(uses_gfx_debug);
#else
			debug(LOG_FATAL, "Warzone was not compiled with the Vulkan backend enabled. Aborting.");
			abort();
#endif
			break;
	}
	ASSERT(current_backend_context != nullptr, "Failed to initialize gfx backend context");
	return gfx_api::context::get()._initialize(impl, antialiasing, swapMode);
}

gfx_api::context& gfx_api::context::get()
{
	return *current_backend_context;
}

// MARK: - Image Format runtime conversion

#include <ProcessDxtc.hpp>
#include <ProcessRGB.hpp>

enum EtcPackFormat
{
	etc1,
	etc2_rgb,
	etc2_rgba,
	dxt1,
	dxt5
};

static void convertRGBtoRGBA(const void* data, gfx_api::context::ImageSize imageSize, std::unique_ptr<uint8_t[]>& outputData, size_t& outputLen)
{
	auto* srcMem = reinterpret_cast<const uint8_t*>(data);
	const uint32_t inputDataChannels = 3;
	const uint32_t outputDataChannels = 4;
	outputLen = imageSize.width * imageSize.height * 4;
	outputData = std::unique_ptr<uint8_t[]>(new uint8_t[outputLen]);
	for (uint32_t row = 0; row < imageSize.height; row++)
	{
		for (uint32_t col = 0; col < imageSize.width; col++)
		{
			uint32_t byte = 0;
			for (; byte < inputDataChannels; byte++)
			{
				const auto& texel = srcMem[(row * imageSize.width + col) * inputDataChannels + byte];
				outputData[(row * imageSize.width  + col) * inputDataChannels + byte] = texel;
			}
			for (; byte < outputDataChannels; byte++)
			{
				outputData[(row * imageSize.width  + col) * outputDataChannels + byte] = 255;
			}
		}
	}
}

bool gfx_api::context::convert2DImageFormat(gfx_api::pixel_format dataFormat, const void* data, size_t dataLen, gfx_api::context::ImageSize imageSize, gfx_api::pixel_format targetFormat, std::unique_ptr<uint8_t[]>& outputData, size_t& outputLen)
{
	if (dataFormat != gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8 && dataFormat != gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8)
	{
		// input format not currently supported
		return false;
	}

	// TODO: ASSERT THAT IMAGE WIDTH AND HEIGHT ARE MULTIPLE OF 4

	EtcPackFormat etcPackFormat = EtcPackFormat::dxt5;
	switch (targetFormat)
	{
		case pixel_format::FORMAT_RGB_S3TC_DXT1:
			etcPackFormat = EtcPackFormat::dxt1;
			break;
		case pixel_format::FORMAT_RGBA_S3TC_DXT5:
			etcPackFormat = EtcPackFormat::dxt5;
			break;
		default:
			return false;
	}

	std::unique_ptr<uint8_t[]> convertedRGBAInput;
	if (dataFormat == gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8)
	{
		// Convert input format to RGBA for compression
		convertRGBtoRGBA(data, imageSize, convertedRGBAInput, dataLen);
		data = convertedRGBAInput.get();
	}

	uint32_t linesToProcess = imageSize.height;
	uint32_t blocks = imageSize.width * linesToProcess / 4;

	size_t outputSize = imageSize.width * imageSize.height / 2;
	if(etcPackFormat == etc2_rgba || etcPackFormat == dxt5) outputSize *= 2;
	std::unique_ptr<uint8_t[]> output = std::unique_ptr<uint8_t[]>(new uint8_t[outputSize]);

	switch (etcPackFormat)
	{
		case etc1:
			CompressEtc1RgbDither(reinterpret_cast<const uint32_t*>(data), reinterpret_cast<uint64_t*>(&(output.get()[0])), blocks, imageSize.width);
			break;
		case etc2_rgb:
			CompressEtc2Rgb(reinterpret_cast<const uint32_t*>(data), reinterpret_cast<uint64_t*>(&(output.get()[0])), blocks, imageSize.width);
			break;
		case etc2_rgba:
			CompressEtc2Rgba(reinterpret_cast<const uint32_t*>(data), reinterpret_cast<uint64_t*>(&(output.get()[0])), blocks, imageSize.width);
			break;
		case dxt1:
			CompressDxt1Dither(reinterpret_cast<const uint32_t*>(data), reinterpret_cast<uint64_t*>(&(output.get()[0])), blocks, imageSize.width);
			break;
		case dxt5:
			CompressDxt5(reinterpret_cast<const uint32_t*>(data), reinterpret_cast<uint64_t*>(&(output.get()[0])), blocks, imageSize.width);
			break;
	}

	return true;
}
