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

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: '<function-style-cast>': unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vk<...>'
#endif
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include <vulkan/vulkan.hpp>
#include "3rdparty/vulkan_hpp_dispatchloaderdynamic.h"
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#include "lib/framework/frame.h"

#include "gfx_api.h"
#include <algorithm>
#include <sstream>
#include <map>
#include <vector>
#include <unordered_map>

#include "3rdparty/optional.hpp"
using nonstd::optional;

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: '<function-style-cast>': unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vk<...>'
#endif
#include "3rdparty/vkh_renderpasscompat.hpp"
#include "3rdparty/vkh_info.hpp"
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#if VK_HEADER_VERSION <= 108
// Use the DispatchLoaderDynamic from 108
using VKDispatchLoaderDynamic = WZ_vk::DispatchLoaderDynamic;
#else
using VKDispatchLoaderDynamic = vk::DispatchLoaderDynamic;
#endif

namespace gfx_api
{
	class backend_Vulkan_Impl
	{
	public:
		backend_Vulkan_Impl() {};
		virtual ~backend_Vulkan_Impl() {};

		virtual PFN_vkGetInstanceProcAddr getVkGetInstanceProcAddr() = 0;
		virtual bool getRequiredInstanceExtensions(std::vector<const char*> &output) = 0;
		virtual bool createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) = 0;

		// Use this function to get the size of the window's underlying drawable dimensions in pixels. This is used for setting viewport sizes, scissor rectangles, and other places where the a VkExtent might show up in relation to the window.
		virtual void getDrawableSize(int* w, int* h) = 0;
	};
}

namespace WZ_vk {
	using UniqueBuffer = vk::UniqueHandle<vk::Buffer, VKDispatchLoaderDynamic>;
	using UniqueDeviceMemory = vk::UniqueHandle<vk::DeviceMemory, VKDispatchLoaderDynamic>;
	using UniqueImage = vk::UniqueHandle<vk::Image, VKDispatchLoaderDynamic>;
	using UniqueImageView = vk::UniqueHandle<vk::ImageView, VKDispatchLoaderDynamic>;
	using UniqueSemaphore = vk::UniqueHandle<vk::Semaphore, VKDispatchLoaderDynamic>;
}

struct circularHostBuffer
{
	vk::Buffer buffer;
	vk::DeviceMemory memory;
	vk::Device &dev;
	uint32_t gpuReadLocation;
	uint32_t hostWriteLocation;
	uint32_t size;

	circularHostBuffer(vk::Device &d, vk::PhysicalDeviceMemoryProperties memprops, uint32_t s, const VKDispatchLoaderDynamic& vkDynLoader, const vk::BufferUsageFlags& usageFlags);
	~circularHostBuffer();

private:
	static bool isBetween(uint32_t rangeBegin, uint32_t rangeEnd, uint32_t position);
	static std::tuple<uint32_t, uint32_t> getWritePosAndNewWriteLocation(uint32_t currentWritePos, uint32_t amount, uint32_t totalSize, uint32_t align);
public:
	uint32_t alloc(uint32_t amount, uint32_t align);

private:
	const VKDispatchLoaderDynamic *pVkDynLoader;
};

struct VkPSO; // forward-declare

struct perFrameResources_t
{
	vk::Device dev;
	vk::DescriptorPool descriptorPool;
	uint32_t numalloc;
	vk::CommandPool pool;
	vk::CommandBuffer cmdDraw;
	vk::CommandBuffer cmdCopy;
	vk::Fence previousSubmission;
	std::vector<WZ_vk::UniqueBuffer> buffer_to_delete;
	std::vector<WZ_vk::UniqueImage> image_to_delete;
	std::vector<WZ_vk::UniqueImageView> image_view_to_delete;
	std::vector<WZ_vk::UniqueDeviceMemory> memory_to_free;

	vk::Semaphore imageAcquireSemaphore;
	vk::Semaphore renderFinishedSemaphore;

	std::unordered_map<VkPSO *, vk::DescriptorSet> perPSO_dynamicUniformBufferDescriptorSets;

	perFrameResources_t( const perFrameResources_t& other ) = delete; // non construction-copyable
	perFrameResources_t& operator=( const perFrameResources_t& ) = delete; // non copyable

	perFrameResources_t(vk::Device& _dev, const uint32_t& graphicsQueueIndex, const VKDispatchLoaderDynamic& vkDynLoader);
	void clean();
	~perFrameResources_t();

private:
	const VKDispatchLoaderDynamic *pVkDynLoader;
};

struct buffering_mechanism
{
	static std::unique_ptr<circularHostBuffer> dynamicUniformBuffer; // NEW
	static void * pDynamicUniformBufferMapped;

	static std::unique_ptr<circularHostBuffer> scratchBuffer;

	static std::vector<std::unique_ptr<perFrameResources_t>> perFrameResources;

	static size_t currentFrame;

	static perFrameResources_t& get_current_resources();
	static perFrameResources_t* get_current_resources_pt();
	static void init(vk::Device dev, size_t swapchainImageCount, const uint32_t& graphicsQueueFamilyIndex, const VKDispatchLoaderDynamic& vkDynLoader);
	static void destroy(vk::Device dev, const VKDispatchLoaderDynamic& vkDynLoader);
	static void swap(vk::Device dev, const VKDispatchLoaderDynamic& vkDynLoader);
};

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
						 VkDebugReportFlagsEXT flags,
						 VkDebugReportObjectTypeEXT objType,
						 uint64_t srcObject,
						 std::size_t location,
						 int32_t msgCode,
						 const char* pLayerPrefix,
						 const char* pMsg,
						 void* pUserData);

struct VkPSOId final : public gfx_api::pipeline_state_object
{
public:
	size_t psoID = 0;
public:
	VkPSOId(size_t psoID) : psoID(psoID) {}
	~VkPSOId() {}
};

struct gfxapi_PipelineCreateInfo
{
	gfx_api::state_description state_desc;
	SHADER_MODE shader_mode;
	gfx_api::primitive_type primitive;
	std::vector<gfx_api::texture_input> texture_desc;
	std::vector<gfx_api::vertex_buffer> attribute_descriptions;

	gfxapi_PipelineCreateInfo(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode, const gfx_api::primitive_type& primitive,
							  const std::vector<gfx_api::texture_input>& texture_desc,
							  const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
	: state_desc(state_desc)
	, shader_mode(shader_mode)
	, primitive(primitive)
	, texture_desc(texture_desc)
	, attribute_descriptions(attribute_descriptions)
	{}
};

struct VkPSO final
{
	vk::Pipeline object;
	vk::DescriptorSetLayout cbuffer_set_layout;
	vk::DescriptorSetLayout textures_set_layout;
	vk::PipelineLayout layout;
	vk::ShaderModule vertexShader;
	vk::ShaderModule fragmentShader;
	vk::Device dev;
	const VKDispatchLoaderDynamic* pVkDynLoader;
	std::vector<vk::Sampler> samplers;

	std::shared_ptr<VkhRenderPassCompat> renderpass_compat;

private:
	// Read shader into text buffer
	static std::vector<uint32_t> readShaderBuf(const std::string& name);

	vk::ShaderModule get_module(const std::string& name, const VKDispatchLoaderDynamic& vkDynLoader);

	static std::array<vk::PipelineShaderStageCreateInfo, 2> get_stages(const vk::ShaderModule& vertexModule, const vk::ShaderModule& fragmentModule);

	static std::array<vk::PipelineColorBlendAttachmentState, 1> to_vk(const REND_MODE& blend_state, const uint8_t& color_mask);

	static vk::PipelineDepthStencilStateCreateInfo to_vk(DEPTH_MODE depth_mode, const gfx_api::stencil_mode& stencil);

	static vk::PipelineRasterizationStateCreateInfo to_vk(const bool& offset, const gfx_api::cull_mode& cull);

	static vk::Format to_vk(const gfx_api::vertex_attribute_type& type);

	static vk::SamplerCreateInfo to_vk(const gfx_api::sampler_type& type);

	static vk::PrimitiveTopology to_vk(const gfx_api::primitive_type& primitive);

public:
	VkPSO(vk::Device _dev,
		  const vk::PhysicalDeviceLimits& limits,
		  const gfxapi_PipelineCreateInfo& createInfo,
		  vk::RenderPass rp,
		  const std::shared_ptr<VkhRenderPassCompat>& renderpass_compat,
		  vk::SampleCountFlagBits rasterizationSamples,
		  const VKDispatchLoaderDynamic& _vkDynLoader
		  );

	~VkPSO();

	VkPSO(vk::Pipeline&& p);
	VkPSO(const VkPSO&) = default;
	VkPSO(VkPSO&&) = default;
	VkPSO& operator=(VkPSO&&) = default;

};

struct VkRoot; // forward-declare

struct VkBuf final : public gfx_api::buffer
{
	vk::Device dev;
	gfx_api::buffer::usage usage;
	WZ_vk::UniqueBuffer object;
	WZ_vk::UniqueDeviceMemory memory;
	size_t buffer_size = 0;

	VkBuf(vk::Device _dev, const gfx_api::buffer::usage&, const VkRoot& root);

	virtual ~VkBuf() override;

	virtual void upload(const size_t & size, const void * data) override;
	virtual void update(const size_t & start, const size_t & size, const void * data) override;

	virtual void bind() override;

private:
	void allocateBufferObject(const std::size_t& width);

private:
	const VkRoot* root;
};

struct VkTexture final : public gfx_api::texture
{
	vk::Device dev;
	WZ_vk::UniqueImage object;
	WZ_vk::UniqueImageView view;
	WZ_vk::UniqueDeviceMemory memory;
	vk::Format internal_format;
	size_t mipmap_levels;

	static size_t format_size(const gfx_api::pixel_format& format);

	static size_t format_size(const vk::Format& format);

	VkTexture(const VkRoot& root, const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const vk::Format& _internal_format, const std::string& filename);

	virtual ~VkTexture() override;

	virtual void bind() override;

	virtual void upload(const std::size_t& mip_level, const std::size_t& offset_x, const std::size_t& offset_y, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& buffer_format, const void* data) override;
	virtual void upload_and_generate_mipmaps(const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const gfx_api::pixel_format& buffer_format, const void* data) override;
	virtual unsigned id() override;

	VkTexture( const VkTexture& other ) = delete; // non construction-copyable
	VkTexture& operator=( const VkTexture& ) = delete; // non copyable

private:
	const VkRoot* root;
};

struct QueueFamilyIndices
{
	optional<uint32_t> graphicsFamily;
	optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct VkRoot final : gfx_api::context
{
	std::unique_ptr<gfx_api::backend_Vulkan_Impl> backend_impl;
	VkhInfo debugInfo;

	vk::Instance inst;
	std::vector<const char*> layers;
	VKDispatchLoaderDynamic vkDynLoader;

	// physical device (and info)
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physDeviceProps;
	vk::PhysicalDeviceFeatures physDeviceFeatures;
	vk::PhysicalDeviceMemoryProperties memprops;
	bool supports_rgb = false;

	QueueFamilyIndices queueFamilyIndices;
	vk::Device dev;
	vk::SurfaceKHR surface;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	bool queueSupportsTimestamps = false;

	// swapchain
	vk::Extent2D swapchainSize;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::SwapchainKHR swapchain;
	uint32_t currentSwapchainIndex = 0;
	std::vector<vk::ImageView> swapchainImageView;

	vk::Image depthStencilImage;
	vk::DeviceMemory depthStencilMemory;
	vk::ImageView depthStencilView;

	// default renderpass
	vk::RenderPass rp;
	std::shared_ptr<VkhRenderPassCompat> rp_compat_info;
	std::vector<vk::Framebuffer> fbo;

	// default texture
	VkTexture* pDefaultTexture = nullptr;

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = nullptr;
	PFN_vkDebugReportMessageEXT dbgBreakCallback = nullptr;
	VkDebugReportCallbackEXT msgCallback = 0;

	std::vector<std::pair<const gfxapi_PipelineCreateInfo, VkPSO *>> createdPipelines;
	VkPSO* currentPSO = nullptr;

	bool debugLayer = false;

	const size_t maxErrorHandlingDepth = 10;
	std::vector<vk::Result> errorHandlingDepth;

public:
	VkRoot(bool _debug);
	~VkRoot();

	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode, const gfx_api::primitive_type& primitive,
															const std::vector<gfx_api::texture_input>& texture_desc,
															const std::vector<gfx_api::vertex_buffer>& attribute_descriptions) override;

	virtual bool initialize(const gfx_api::backend_Impl_Factory& impl) override;
	virtual void draw(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&) override;
	virtual void draw_elements(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&, const gfx_api::index_type&) override;
	virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void unbind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void disable_all_vertex_buffers() override;
	virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) override;

	virtual gfx_api::texture* create_texture(const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::pixel_format& internal_format, const std::string& filename = "") override;

	virtual gfx_api::buffer * create_buffer_object(const gfx_api::buffer::usage &usage, const buffer_storage_hint& hint = buffer_storage_hint::static_draw) override;

private:

	std::vector<vk::DescriptorSet> allocateDescriptorSets(vk::DescriptorSetLayout arg);

	bool createVulkanInstance(std::vector<const char*> extensions, std::vector<const char*> layers, PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr);
	vk::PhysicalDevice pickPhysicalDevice();

	bool createSurface();
	// pickPhysicalDevice();
	void getQueueFamiliesInfo();
	bool createLogicalDevice();
	void getQueues();
	bool createSwapchain();
	void rebuildPipelinesIfNecessary();

	void createDefaultRenderpass(vk::Format swapchainFormat, vk::Format depthFormat);
	void setupSwapchainImages();

	vk::Format get_format(const gfx_api::pixel_format& format);

private:
	static vk::IndexType to_vk(const gfx_api::index_type& index);

public:
	virtual void bind_index_buffer(gfx_api::buffer& index_buffer, const gfx_api::index_type& index) override;
	virtual void unbind_index_buffer(gfx_api::buffer&) override;

	virtual void bind_textures(const std::vector<gfx_api::texture_input>& attribute_descriptions, const std::vector<gfx_api::texture*>& textures) override;

public:
	virtual void set_constants(const void* buffer, const std::size_t& size) override;

	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso, bool notextures) override;

	virtual void flip(int clearMode) override;
	virtual void set_polygon_offset(const float& offset, const float& slope) override;
	virtual void set_depth_range(const float& min, const float& max) override;
private:
	void startRenderPass();

	enum AcquireNextSwapchainImageResult
	{
		eSuccess,
		eRecoveredFromError,
		eUnhandledFailure
	};
	AcquireNextSwapchainImageResult acquireNextSwapchainImage();

	bool handleSurfaceLost();
	void destroySwapchainAndSwapchainSpecificStuff(bool doDestroySwapchain);
	bool createNewSwapchainAndSwapchainSpecificStuff(const vk::Result& reason);

public:
	virtual int32_t get_context_value(const context_value property) override;
	virtual void debugStringMarker(const char *str) override;
	virtual void debugSceneBegin(const char *descr) override;
	virtual void debugSceneEnd(const char *descr) override;
	virtual bool debugPerfAvailable() override;
	virtual bool debugPerfStart(size_t sample) override;
	virtual void debugPerfStop() override;
	virtual void debugPerfBegin(PERF_POINT pp, const char *descr) override;
	virtual void debugPerfEnd(PERF_POINT pp) override;
	virtual uint64_t debugGetPerfValue(PERF_POINT pp) override;
	virtual std::map<std::string, std::string> getBackendGameInfo() override;
	virtual bool getScreenshot(iV_Image &output) override;
	virtual void handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;
	virtual void shutdown() override;
};

#endif // defined(WZ_VULKAN_ENABLED)
