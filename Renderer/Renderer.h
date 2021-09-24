#pragma once

#include <memory>
#include <vector>
#include <cassert>
#include "../aveng_window.h"
#include "../EngineDevice.h"
#include "../swapchain.h"
#include "../cool.h"
#include "../GUI/imgui.h"
#include "../GUI/imgui_impl_glfw.h"
#include "../GUI/imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace aveng {

	class Renderer {

	public:

		Renderer(AvengWindow &window, EngineDevice &device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer &operator=(const Renderer&) = delete;

		// Our app needs to be able to access the swap chain render pass in order to configure any pipelines it creates
		VkRenderPass getSwapChainRenderPass() const { return aveng_swapchain->getRenderPass(); }
		float getAspectRatio() const { return aveng_swapchain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const 
		{
			assert(isFrameStarted && "Cannot get command buffer. The frame is not in progress.");
			return commandBuffers[currentFrameIndex];
		}

		VkDescriptorSet getCurrentDescriptorSet() const
		{ 
			assert(isFrameStarted && "Cannot get descriptor set. The frame is not in progress.");
			return descriptorSets[currentFrameIndex]; 
		}

		VkDescriptorSetLayout* getDescriptorSetLayout() {
			return &descriptorSetLayout;
		}

		int getFrameIndex() const
		{
			assert(isFrameStarted && "Cannot get the frame index when frame is not in progress.");
			return currentFrameIndex;
		}

		// SwapChain getters
		uint32_t getImageCount() const { return aveng_swapchain->imageCount(); }
		VkImage& getImage(int index) { return aveng_swapchain->getImage(index); }
		VkFormat getSwapChainImageFormat() { return aveng_swapchain->getSwapChainImageFormat(); }

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void createUniformBuffers();
		void updateUniformBuffer(uint32_t currentImage, float frameTime, glm::mat4 p);
		void destroyUniformBuffers();

	private:

		VkResult err;

		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain(int destroy_uniform_buffers = 0);
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSets();

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
		VkDescriptorSetLayout descriptorSetLayout;

		AvengWindow& aveng_window;
		EngineDevice& engineDevice;

		VkCommandBuffer imGuiCommandBuffer;
		std::vector<VkCommandBuffer> commandBuffers;
		VkCommandPool imGuiCommandPool;

		// SwapChain aveng_swapchain{ engineDevice, aveng_window.getExtent() };	// previous stack allocated. Ptr makes it easier to rebuild when the window resizes
		std::unique_ptr<SwapChain> aveng_swapchain;
		
		uint32_t currentImageIndex{0};
		int currentFrameIndex; // Not tied to the image index
		bool isFrameStarted{ false };
		
		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBuffersMemory;

	};

}


/* Note */
// The Graphics API - Previously stack allocated
//GFXPipeline gfxPipeline{
//	engineDevice, 
//	"shaders/simple_shader.vert.spv", 
//	"shaders/simple_shader.frag.spv", 
//	GFXPipeline::defaultPipelineConfig(WIDTH, HEIGHT) 
//};