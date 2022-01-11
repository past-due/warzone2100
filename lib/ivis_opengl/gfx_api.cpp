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
#include "gfx_api_image_compress_priv.h"

static gfx_api::backend_type backend = gfx_api::backend_type::opengl_backend;
bool uses_gfx_debug = false;
static gfx_api::context* current_backend_context = nullptr;

static uint8_t inverseGammaCorrectionTable[UINT8_MAX + 1] = {};
static uint8_t gammaCorrectionTable[UINT8_MAX + 1] = {};
static float gammaCorrectionTable_f[UINT8_MAX + 1] = {};

inline float D3DX_SRGB_to_FLOAT_inexact(float val)
{
	if( val < 0.04045f )
		val /= 12.92f;
	else
		val = std::pow((val + 0.055f)/1.055f,2.4f);
	return val;
}

inline double D3DX_SRGB_to_DOUBLE_inexact(double val)
{
	if( val <= 0.04045 )
		val /= 12.92;
	else
		val = std::pow((val + 0.055)/1.055,2.4);
	return val;
}

void buildGammaCorrectionTable()
{
	constexpr float sRGB_Gamma = 2.2f; // Could make this configurable in the future...
	float gamma = sRGB_Gamma;
	float gamma_inverse = 1.f / gamma;
	double num_colors = 255;
	for (uint16_t i = 0; i <= UINT8_MAX; i++)
	{
		inverseGammaCorrectionTable[i] = std::pow(i / num_colors, gamma_inverse) * num_colors;
//		gammaCorrectionTable[i] = std::pow(i / num_colors, gamma) * num_colors;
		gammaCorrectionTable[i] = D3DX_SRGB_to_DOUBLE_inexact(i / num_colors) * num_colors;
		gammaCorrectionTable_f[i] = D3DX_SRGB_to_DOUBLE_inexact(i / num_colors);
	}
}

/// \brief Perform the inverse gamma companding for a sRGB color
	/// \details http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	inline glm::vec3 perform_inverse_sRGB_companding(const glm::vec3& srgb_color)
	{
		glm::vec3 linear_srgb_color;
		for (int i : {0, 1, 2})
		{
			linear_srgb_color[i] =
				(srgb_color[i] <= 0.04045) ? srgb_color[i] / 12.92 : std::pow((srgb_color[i] + 0.055) / 1.055, 2.4);
		}
		return linear_srgb_color;
	}

	/// \param rgb_color A color represented in sRGB (D65)
	/// \return A color represented in CIEXYZ (D65)
	inline glm::vec3 convert_RGB_to_XYZ(const glm::vec3& srgb_color)
	{
		// Inverse companding for sRGB
		const glm::vec3 linear_srgb = perform_inverse_sRGB_companding(srgb_color);

		// Retrieved from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		// sRGB (D65)
//		constexpr double M_data[3 * 3] = {
//			0.4124564, 0.3575761, 0.1804375, 0.2126729, 0.7151522, 0.0721750, 0.0193339, 0.1191920, 0.9503041};

		glm::mat3x3 M = {
			0.4124564, 0.3575761, 0.1804375, 0.2126729, 0.7151522, 0.0721750, 0.0193339, 0.1191920, 0.9503041};

//		// Note: The use of "auto" avoids unnecessary data copy by lazy evaluation
//		const auto M = Eigen::Map<const Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(M_data);

		return 100.0f * M * linear_srgb;
	}

	/// \param xyz_color A color represented in CIEXYZ (D65)
	/// \return A color represented in sRGB, unclipped (D65)
	inline glm::vec3 convert_XYZ_to_RGB(const glm::vec3& xyz_color)
	{
		// Retrieved from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		// sRGB (D65)
		glm::mat3x3 M = {
			3.2404542, -1.5371385, -0.4985314, -0.9692660, 1.8760108, 0.0415560, 0.0556434, -0.2040259, 1.0572252};

//		// Note: The use of "auto" avoids unnecessary data copy by lazy evaluation
//		const auto M = Eigen::Map<const Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(M_data);

		// Inverse companding for sRGB
		const glm::vec3 linear_srgb = (M * xyz_color) / 100.0f;

		return linear_srgb; //sRGB_companding(linear_srgb);
	}

	inline glm::vec3 convert_sRGB_to_RGB_perceptual(const glm::vec3& srgb_color)
	{
		return convert_XYZ_to_RGB(convert_RGB_to_XYZ(srgb_color));
	}

bool gfx_api::context::initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode swapMode, gfx_api::backend_type backendType)
{
	buildGammaCorrectionTable();

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
	bool result = gfx_api::context::get()._initialize(impl, antialiasing, swapMode);
	if (!result)
	{
		return false;
	}

	// Calculate the best available run-time compression formats
	gfx_api::initBestRealTimeCompressionFormats();

	return result;
}

gfx_api::context& gfx_api::context::get()
{
	return *current_backend_context;
}

#include "png_util.h"

static gfx_api::texture* loadImageTextureFromFile_PNG(const std::string& filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	iV_Image loadedUncompressedImage;

	// Check that at least base RGBA (sRGB) support is available
//	bool srgb_supported = gfx_api::context::get().texture2DFormatIsSupported(gfx_api::pixel_format::FORMAT_RGBA8_SRGB_PACK8, gfx_api::pixel_format_usage::sampled_image);
	bool srgb_supported = gfx_api::context::get().getFrameBufferColorspace() == iV_Image::ColorSpace::sRGB;
	bool can_use_srgb = false;
	switch (textureType)
	{
		case gfx_api::texture_type::game_texture:
		case gfx_api::texture_type::user_interface:
			can_use_srgb = true;
			break;
		case gfx_api::texture_type::alpha_mask:
			can_use_srgb = true;
			break;
		default:
			can_use_srgb = false;
			break;
	}
	iV_Image::ColorSpace loadingColorspace = (can_use_srgb && srgb_supported) ? iV_Image::ColorSpace::sRGB : iV_Image::ColorSpace::Linear;

	// 1.) Load the PNG into an iV_Image
	if (!iV_loadImage_PNG2(filename.c_str(), loadedUncompressedImage, loadingColorspace))
	{
		// Failed to load the image
		return nullptr;
	}

	return gfx_api::context::get().loadTextureFromUncompressedImage(std::move(loadedUncompressedImage), textureType, filename, maxWidth, maxHeight);
}

std::string imageLoadFilenameFromInputFilename(const char *filename)
{
	ASSERT_OR_RETURN("", filename != nullptr, "Null filename");
	// For now, always return the input filename
	return filename;
}

//#include "piepalette.h"
//const PIELIGHT transparentWindow = pal_RGBA(16, 16, 128, 128);

glm::vec4 gfx_api::context::gammaCorrectColorVec4_Slow(glm::vec4 color) const
{
	const bool needsGammaCorrection = (getFrameBufferColorspace() == iV_Image::ColorSpace::sRGB);
	if (needsGammaCorrection)
	{
		color[0] = D3DX_SRGB_to_FLOAT_inexact(color[0]);
		color[1] = D3DX_SRGB_to_FLOAT_inexact(color[1]);
		color[2] = D3DX_SRGB_to_FLOAT_inexact(color[2]);
	}
	return color;
}

glm::vec4 gfx_api::context::pielightToVec4ForShaders(PIELIGHT color) const
{
	const bool needsGammaCorrection = (getFrameBufferColorspace() == iV_Image::ColorSpace::sRGB);
	if (needsGammaCorrection)
	{
//		if (color.rgba == transparentWindow.rgba)
//		{
//			return pal_RGBA(13,0,109,128);
//		}

//		return glm::vec4(perform_inverse_sRGB_companding(glm::vec3(color.byte.r / 255.f, color.byte.g / 255.f, color.byte.b / 255.f)), color.byte.a / 255.f);

		glm::vec4 gammaCorrectedColor;
		gammaCorrectedColor[0] = gammaCorrectionTable_f[color.byte.r];
		gammaCorrectedColor[1] = gammaCorrectionTable_f[color.byte.g];
		gammaCorrectedColor[2] = gammaCorrectionTable_f[color.byte.b];
		gammaCorrectedColor[3] = color.byte.a / 255.f;
		return gammaCorrectedColor;

//		PIELIGHT gammaCorrectedColor = color;
//		gammaCorrectedColor.byte.r = gammaCorrectionTable[gammaCorrectedColor.byte.r];
//		gammaCorrectedColor.byte.g = gammaCorrectionTable[gammaCorrectedColor.byte.g];
//		gammaCorrectedColor.byte.b = gammaCorrectionTable[gammaCorrectedColor.byte.b];
//		gammaCorrectedColor.byte.a = inverseGammaCorrectionTable[gammaCorrectedColor.byte.a];

//		auto rgbVec3 = convert_sRGB_to_RGB_perceptual(glm::vec3(gammaCorrectedColor.byte.r / 255.f, gammaCorrectedColor.byte.g / 255.f, gammaCorrectedColor.byte.b / 255.f));
//		PIELIGHT rgbClr;
//		rgbClr.byte.r = rgbVec3[0] * 255.f;
//		rgbClr.byte.g = rgbVec3[1] * 255.f;
//		rgbClr.byte.b = rgbVec3[2] * 255.f;
//		rgbClr.byte.a = color.byte.a;
//		return gammaCorrectedColor;
	}
	else
	{
		return glm::vec4(color.vector[0] / 255.f, color.vector[1] / 255.f, color.vector[2] / 255.f, color.vector[3] / 255.f);
//		return color;
	}
}

// MARK: - High-level texture loading

// Load a texture from a file
// (which loads straight to a texture based on the appropriate texture_type, handling mip_maps, compression, etc)
gfx_api::texture* gfx_api::context::loadTextureFromFile(const char *filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	std::string imageLoadFilename = imageLoadFilenameFromInputFilename(filename);

	if (strEndsWith(imageLoadFilename, ".png"))
	{
		return loadImageTextureFromFile_PNG(imageLoadFilename, textureType, maxWidth, maxHeight);
	}
	else
	{
		debug(LOG_ERROR, "Unable to load image file: %s", filename);
		return nullptr;
	}
}

// Takes an iv_Image and texture_type and loads a texture as appropriate / possible
gfx_api::texture* gfx_api::context::loadTextureFromUncompressedImage(iV_Image&& image, gfx_api::texture_type textureType, const std::string& filename, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	// 1.) Convert to expected # of channels based on textureType
	switch (textureType)
	{
		case gfx_api::texture_type::specular_map:
		{
			bool result = image.convert_to_luma();
			ASSERT_OR_RETURN(nullptr, result, "(%s): Failed to convert specular map", filename.c_str());
			break;
		}
		case gfx_api::texture_type::alpha_mask:
		{
			if (image.channels() > 1)
			{
				ASSERT_OR_RETURN(nullptr, image.channels() == 4, "(%s): Alpha mask does not have 1 or 4 channels, as expected", filename.c_str());
				image.convert_to_single_channel(3); // extract alpha channel
			}
			break;
		}
		default:
			break;
	}

	// 2.) If maxWidth / maxHeight exceed current image dimensions, resize()
	image.scale_image_max_size(maxWidth, maxHeight);

	// 3.) Determine mipmap levels (if needed / desired)
	bool generateMipMaps = (textureType != gfx_api::texture_type::user_interface);
	size_t mipmap_levels = 1;
	if (generateMipMaps)
	{
		// Calculate how many mip-map levels (with a target minimum level dimension of 4)
		mipmap_levels = static_cast<size_t>(floor(log2(std::max(image.width(), image.height()))));
		if (mipmap_levels > 2)
		{
			mipmap_levels = (mipmap_levels - 2) + 1 /* for original level */;
		}
		else
		{
			// just use the original level, which must be small
			mipmap_levels = 1;
		}
	}

	// 4.) Determine uploadFormat
	auto uploadFormat = image.pixel_format();
	auto bestAvailableCompressedFormat = gfx_api::bestRealTimeCompressionFormatForImage(image, textureType);
	if (bestAvailableCompressedFormat.has_value() && bestAvailableCompressedFormat.value() != gfx_api::pixel_format::invalid)
	{
		// For now, check that the minimum mipmap level is 4x4 or greater, otherwise do not run-time compress
		size_t min_mipmap_w = std::max<size_t>(1, image.width() >> (mipmap_levels - 1));
		size_t min_mipmap_h = std::max<size_t>(1, image.height() >> (mipmap_levels - 1));
		if (min_mipmap_w >= 4 && min_mipmap_h >= 4)
		{
			uploadFormat = bestAvailableCompressedFormat.value();
		}
	}

	// 5.) Extend channels, if needed, to a supported uncompressed format
	if (is_uncompressed_format(uploadFormat))
	{
		auto channels = image.channels();
		auto colorSpace = image.colorSpace();
		// Verify that the gfx backend supports this format
		auto closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(channels, colorSpace);
		if (!closestSupportedChannels.has_value() && colorSpace == iV_Image::ColorSpace::sRGB)
		{
			// Try again, but with a linear colorSpace
			// (This is non-ideal and will result in some precision issues, but at least it'll display something)
			colorSpace = iV_Image::ColorSpace::Linear;
			closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(channels, colorSpace);
		}
		ASSERT_OR_RETURN(nullptr, closestSupportedChannels.has_value(), "Exhausted all possible uncompressed formats??");
		for (auto i = image.channels(); i < closestSupportedChannels; ++i)
		{
			image.expand_channels_towards_rgba();
		}
		uploadFormat = iV_Image::pixel_format_for_channels(closestSupportedChannels.value(), colorSpace);
	}

	// 6.) Create a new compatible gpu texture object
	std::unique_ptr<gfx_api::texture> pTexture = std::unique_ptr<gfx_api::texture>(gfx_api::context::get().create_texture(mipmap_levels, image.width(), image.height(), uploadFormat, filename));

	// 7.) Upload initial (full) level
	if (uploadFormat == image.pixel_format())
	{
		bool uploadResult = pTexture->upload(0, image);
		ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
	}
	else
	{
		// Run-time compression
		auto compressedImage = gfx_api::compressImage(image, uploadFormat);
		ASSERT_OR_RETURN(nullptr, compressedImage != nullptr, "Failed to compress image to format: %zu", static_cast<size_t>(uploadFormat));
		bool uploadResult = pTexture->upload(0, *compressedImage);
		ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
	}

	// 8.) Generate and upload mipmaps (if needed)
	for (size_t i = 1; i < mipmap_levels; i++)
	{
		unsigned int output_w = std::max<unsigned int>(1, image.width() >> 1);
		unsigned int output_h = std::max<unsigned int>(1, image.height() >> 1);
//		debug(LOG_INFO, "Mipmap level %zu, size: %u x %u", i, output_w, output_h);

		image.resize(output_w, output_h);

		if (uploadFormat == image.pixel_format())
		{
			bool uploadResult = pTexture->upload(i, image);
			ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
		}
		else
		{
			// Run-time compression
			auto compressedImage = gfx_api::compressImage(image, uploadFormat);
			ASSERT_OR_RETURN(nullptr, compressedImage != nullptr, "Failed to compress image to format: %zu", static_cast<size_t>(uploadFormat));
			bool uploadResult = pTexture->upload(i, *compressedImage);
			ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
		}
	}

	return pTexture.release();
}

// MARK: - texture

optional<unsigned int> gfx_api::context::getClosestSupportedUncompressedImageFormatChannels(unsigned int channels, iV_Image::ColorSpace colorSpace)
{
	auto format = iV_Image::pixel_format_for_channels(channels, colorSpace);

	// Verify that the gfx backend supports this format
	while (!gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::flags::sampled_image))
	{
		ASSERT_OR_RETURN(nullopt, channels < 4, "Exhausted all possible uncompressed formats??");
		channels += 1;
		format = iV_Image::pixel_format_for_channels(channels, colorSpace);
	}

	return channels;
}

gfx_api::texture* gfx_api::context::createTextureForCompatibleImageUploads(const size_t& mipmap_count, const iV_Image& bitmap, const std::string& filename)
{
	auto channels = bitmap.channels();
	auto colorSpace = bitmap.colorSpace();

	// Verify that the gfx backend supports this format
	auto closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(channels, colorSpace);
	if (!closestSupportedChannels.has_value() && colorSpace == iV_Image::ColorSpace::sRGB)
	{
		// Try again, but with a linear colorSpace
		// (This is non-ideal and will result in some precision issues, but at least it'll display something)
		colorSpace = iV_Image::ColorSpace::Linear;
		closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(channels, colorSpace);
	}
	ASSERT_OR_RETURN(nullptr, closestSupportedChannels.has_value(), "Exhausted all possible uncompressed formats??");

	auto target_pixel_format = iV_Image::pixel_format_for_channels(closestSupportedChannels.value(), colorSpace);
//	if (!gfx_api::context::get().texture2DFormatIsSupported(target_pixel_format, gfx_api::pixel_format_usage::flags::sampled_image))
//	{
//		if (bitmap.colorSpace() == iV_Image::ColorSpace::sRGB)
//		{
//			// TODO: Better handling of sRGB here - ideally we would want to convert to linear space (even if this is non-ideal) if there are no sRGB formats supported...
//		}
//		else
//		{
//			debug(LOG_ERROR, "Unable to find a supported pixel format for image upload?? (channels: %u, colorspace: %u)", channels, (unsigned)bitmap.colorSpace());
//			return nullptr;
//		}
//	}

	gfx_api::texture* pTexture = gfx_api::context::get().create_texture(1, bitmap.width(), bitmap.height(), target_pixel_format, filename);
	return pTexture;
}
