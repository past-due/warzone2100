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

#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include "png_util.h"
#include <spng.h>
#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include <algorithm>

#define PNG_BYTES_TO_CHECK 8

IMGSaveError IMGSaveError::None = IMGSaveError();

// SPNG callbacks
static int wzspng_read_data(spng_ctx *ctx, void *user, void *dest, size_t length)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)user;
	(void)ctx;

	if (fileHandle == nullptr)
	{
		return SPNG_IO_ERROR;
	}

	PHYSFS_sint64 result = WZ_PHYSFS_readBytes(fileHandle, dest, static_cast<PHYSFS_uint32>(length));
	if (result > -1)
	{
		size_t byteCountRead = static_cast<size_t>(result);
		if (byteCountRead == length)
		{
			return 0; // success
		}
		return SPNG_IO_EOF;
	}

	return SPNG_IO_ERROR;
}
// End of SPNG callbacks

bool iV_loadImage_PNG(const char *fileName, iV_Image *image)
{
	// Open file
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	ASSERT_OR_RETURN(false, fileHandle != nullptr, "Could not open %s: %s", fileName, WZ_PHYSFS_getLastError());
	WZ_PHYSFS_SETBUFFER(fileHandle, 4096)//;

	// Create SPNG context
	spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
	if (ctx == NULL) return false;

	int ret = 0;
	// Set read stream function
	ret = spng_set_png_stream(ctx, wzspng_read_data, fileHandle);
	if (ret) goto err;

	struct spng_ihdr ihdr;
	ret = spng_get_ihdr(ctx, &ihdr);
	if (ret) goto err;

	/* Determine output image size */
	size_t image_size;
	ret = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image_size);
	if (ret) goto err;

	if (!image->allocate(ihdr.width, ihdr.height, 4))
	{
		ret = -1;
		goto err;
	}
	if (image->data_size() != image_size)
	{
		ret = -1;
		goto err;
	}

	/* Decode to 8-bit RGBA */
	ret = spng_decode_image(ctx, image->bmp_w(), image->data_size(), SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);
	if (ret) goto err;

err:
	spng_ctx_free(ctx);

	if (fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
	}

	return ret == 0;
}

bool iV_loadImage_PNG2(const char *fileName, iV_Image& image, bool forceRGBA8 /*= false*/)
{
	spng_format fmt = SPNG_FMT_RGBA8;
	int decode_flags = SPNG_DECODE_TRNS;
	unsigned int channels = 4;
	struct spng_ihdr ihdr;
	struct spng_trns trns = {0};
	int have_trns = 0;

	// Open file
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	ASSERT_OR_RETURN(false, fileHandle != nullptr, "Could not open %s: %s", fileName, WZ_PHYSFS_getLastError());
	WZ_PHYSFS_SETBUFFER(fileHandle, 4096)//;

	// Create SPNG context
	spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
	if (ctx == NULL) return false;

	int ret = 0;
	// Set read stream function
	ret = spng_set_png_stream(ctx, wzspng_read_data, fileHandle);
	if (ret) goto err;

	ret = spng_get_ihdr(ctx, &ihdr);
	if (ret) goto err;

	have_trns = !spng_get_trns(ctx, &trns);

	/* Determine output image format */
	switch (ihdr.color_type)
	{
		case SPNG_COLOR_TYPE_GRAYSCALE:
			// WZ doesn't support 16-bit color, and libspng doesn't support converting 16-bit grayscale to anything but SPNG_FMT_G(A)16 or SPNG_FMT_RGB(A)8
			if (!have_trns)
			{
				fmt = (ihdr.bit_depth <= 8) ? SPNG_FMT_G8 : SPNG_FMT_RGB8;
			}
			else
			{
				fmt = (ihdr.bit_depth <= 8) ? SPNG_FMT_GA8 : SPNG_FMT_RGBA8;
			}
			break;
		case SPNG_COLOR_TYPE_TRUECOLOR:
			fmt = (have_trns) ? SPNG_FMT_RGBA8 : SPNG_FMT_RGB8;
			break;
		case SPNG_COLOR_TYPE_INDEXED:
			fmt = (have_trns) ? SPNG_FMT_RGBA8 : SPNG_FMT_RGB8;
			break;
		case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
			// WZ doesn't support 16-bit color, and libspng doesn't support converting 16-bit grayscale-alpha to anything but SPNG_FMT_GA16 or SPNG_FMT_RGB(A)8
			fmt = (ihdr.bit_depth <= 8) ? SPNG_FMT_GA8 : SPNG_FMT_RGBA8;
			break;
		case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
			fmt = SPNG_FMT_RGBA8;
			break;
	}

	if (forceRGBA8)
	{
		/* Must end up with 32bpp, 4 channel RGBA */
		fmt = SPNG_FMT_RGBA8;
	}

	switch (fmt)
	{
		case SPNG_FMT_RGBA8:
			channels = 4;
			break;
		case SPNG_FMT_RGB8:
			channels = 3;
			break;
		case SPNG_FMT_GA8:
			channels = 2;
			break;
		case SPNG_FMT_G8:
			channels = 1;
			break;
		default:
			ret = -1;
			goto err;
	}

	/* Determine output image size */
	size_t image_size;
	ret = spng_decoded_image_size(ctx, fmt, &image_size);
	if (ret) goto err;

	if (!image.allocate(ihdr.width, ihdr.height, channels))
	{
		ret = -1;
		goto err;
	}
	if (image.data_size() != image_size)
	{
		ret = -1;
		goto err;
	}

	/* Decode to 8-bit RGBA */
	ret = spng_decode_image(ctx, image.bmp_w(), image.data_size(), fmt, decode_flags);
	if (ret) goto err;

err:
	spng_ctx_free(ctx);

	if (fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
	}

	return ret == 0;
}

//struct MemoryBufferInputStream
//{
//public:
//	MemoryBufferInputStream(const std::vector<unsigned char> *pMemoryBuffer, size_t currentPos = 0)
//	: pMemoryBuffer(pMemoryBuffer)
//	, currentPos(currentPos)
//	{ }
//
//	size_t readBytes(png_bytep destBytes, size_t byteCountToRead)
//	{
//		const size_t remainingBytes = pMemoryBuffer->size() - currentPos;
//		size_t bytesToActuallyRead = std::min(byteCountToRead, remainingBytes);
//		if (bytesToActuallyRead > 0)
//		{
//			memcpy(destBytes, pMemoryBuffer->data() + currentPos, bytesToActuallyRead);
//		}
//		currentPos += bytesToActuallyRead;
//		return bytesToActuallyRead;
//	}
//
//private:
//	const std::vector<unsigned char> *pMemoryBuffer;
//	size_t currentPos = 0;
//};
//
//static void wzpng_read_data_from_buffer(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
//{
//	if (png_ptr != nullptr)
//	{
//		MemoryBufferInputStream *pMemoryStream = (MemoryBufferInputStream *)png_get_io_ptr(png_ptr);
//		if (pMemoryStream != nullptr)
//		{
//			size_t byteCountRead = pMemoryStream->readBytes(outBytes, byteCountToRead);
//			if (byteCountRead == byteCountToRead)
//			{
//				return;
//			}
//			png_error(png_ptr, "Attempt to read beyond end of data");
//		}
//		png_error(png_ptr, "Invalid memory read");
//	}
//}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
IMGSaveError iV_loadImage_PNG(const std::vector<unsigned char>& memoryBuffer, iV_Image *image)
{
	return IMGSaveError("iV_loadImage_PNG: Unimplemented method");
//	unsigned char PNGheader[PNG_BYTES_TO_CHECK];
//	png_structp png_ptr = nullptr;
//	png_infop info_ptr = nullptr;
//
//	MemoryBufferInputStream inputStream = MemoryBufferInputStream(&memoryBuffer, 0);
//
//	size_t readSize = inputStream.readBytes(PNGheader, PNG_BYTES_TO_CHECK);
//	if (readSize < PNG_BYTES_TO_CHECK)
//	{
//		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
//		return IMGSaveError("iV_loadImage_PNG: Insufficient length for PNG header");
//	}
//
//	// Verify the PNG header to be correct
//	if (png_sig_cmp(PNGheader, 0, PNG_BYTES_TO_CHECK))
//	{
//		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
//		return IMGSaveError("iV_loadImage_PNG: Did not recognize PNG header");
//	}
//
//	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
//	if (png_ptr == nullptr)
//	{
//		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
//		return IMGSaveError("iV_loadImage_PNG: Unable to create png struct");
//	}
//
//	info_ptr = png_create_info_struct(png_ptr);
//	if (info_ptr == nullptr)
//	{
//		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
//		return IMGSaveError("iV_loadImage_PNG: Unable to create png info struct");
//	}
//
//	// Set libpng's failure jump position to the if branch,
//	// setjmp evaluates to false so the else branch will be executed at first
//	if (setjmp(png_jmpbuf(png_ptr)))
//	{
//		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
//		return IMGSaveError("iV_loadImage_PNG: Error decoding PNG data");
//	}
//
//	// Tell libpng how many byte we already read
//	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
//
//	/* Set up the input control */
//	png_set_read_fn(png_ptr, &inputStream, wzpng_read_data_from_buffer);
//
//	// Most of the following transformations are seemingly not needed
//	// Filler is, however, for an unknown reason required for tertilesc[23]
//
//	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
//	png_set_strip_16(png_ptr);
//
//	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
//	 * byte into separate bytes (useful for paletted and grayscale images).
//	 */
//// 	png_set_packing(png_ptr);
//
//	/* More transformations to ensure we end up with 32bpp, 4 channel RGBA */
//	png_set_gray_to_rgb(png_ptr);
//	png_set_palette_to_rgb(png_ptr);
//	png_set_tRNS_to_alpha(png_ptr);
//	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
////	png_set_gray_1_2_4_to_8(png_ptr);
//
//	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
//
//	image->width = png_get_image_width(png_ptr, info_ptr);
//	image->height = png_get_image_height(png_ptr, info_ptr);
//	image->depth = png_get_channels(png_ptr, info_ptr);
//	image->bmp = (unsigned char *)malloc(image->height * png_get_rowbytes(png_ptr, info_ptr));
//
//	{
//		unsigned int i = 0;
//		png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
//		for (i = 0; i < png_get_image_height(png_ptr, info_ptr); i++)
//		{
//			memcpy(image->bmp + (png_get_rowbytes(png_ptr, info_ptr) * i), row_pointers[i], png_get_rowbytes(png_ptr, info_ptr));
//		}
//	}
//
//	PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
//
//	if (image->depth < 3 || image->depth > 4)
//	{
//		IMGSaveError error;
//		error.text = "Unsupported image depth (";
//		error.text += std::to_string(image->depth);
//		error.text += ") found.  We only support 3 (RGB) or 4 (ARGB)";
//		return error;
//	}
//
//	return IMGSaveError::None;
}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
static IMGSaveError internal_saveImage_PNG(const char *fileName, const iV_Image *image, int color_type)
{
	IMGSaveError error;
	error.text = "internal_saveImage_PNG: Unimplemented";
	return error;
}

// Note: This function must be thread-safe.
IMGSaveError iV_saveImage_PNG(const char *fileName, const iV_Image *image)
{
	return internal_saveImage_PNG(fileName, image, 0); //PNG_COLOR_TYPE_RGB);
}

// Note: This function must be thread-safe.
IMGSaveError iV_saveImage_PNG_Gray(const char *fileName, const iV_Image *image)
{
	return internal_saveImage_PNG(fileName, image, 0); //PNG_COLOR_TYPE_GRAY);
}
