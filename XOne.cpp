#pragma once
#include <iostream>
#define LOG(a) std::cout<<a<<std::endl;

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
#include <array>

#include "XOne.h"

namespace aveng {

	struct SimplePushConstantData {
		glm::mat2 transform{1.f};	// a glm::mat2 constructed in this fashion constructs an identity matrix by default
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};


	XOne::XOne() 
	{
		loadAppObjects();
		createPipelineLayout();
		recreateSwapChain();
		createCommandBuffers();
	}

	XOne::~XOne()
	{
		vkDestroyPipelineLayout(engineDevice.device(), pipelineLayout, nullptr);
	}

	void XOne::run()
	{
		// Keep the window open until shouldClose is truthy
		while (!aveng_window.shouldClose()) {
			glfwPollEvents();
			drawFrame();
		}

		// Block until all GPU operations quit.
		vkDeviceWaitIdle(engineDevice.device());
	}

	void XOne::createPipelineLayout() 
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		//  Structure specifying the parameters of a newly created pipeline layout object
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		// used to send additional data, other than vertex information, to our shaders 
		// such as textures and uniform buffer objects
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(engineDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout.");
		}
	}

	//void XOne::sierpinski(
	//	std::vector<AvengModel::Vertex>& vertices,
	//	int depth,
	//	glm::vec2 left,
	//	glm::vec2 right,
	//	glm::vec2 top
	//) {
	//	if (depth <= 0) {
	//		vertices.push_back({ top });
	//		vertices.push_back({ right });
	//		vertices.push_back({ left });
	//	}
	//	else {
	//		auto leftTop = 0.5f * (left + top);
	//		auto rightTop = 0.5f * (right + top);
	//		auto leftRight = 0.5f * (left + right);
	//		sierpinski(vertices, depth - 1, left, leftRight, leftTop);
	//		sierpinski(vertices, depth - 1, leftRight, right, rightTop);
	//		sierpinski(vertices, depth - 1, leftTop, rightTop, top);
	//	}
	//}

	//void XOne::loadModels() {
	//	std::vector<AvengModel::Vertex> vertices{};
	//	sierpinski(vertices, 5, { -0.5f, 0.5f }, { 0.5f, 0.5f }, { 0.0f, -0.5f });
	//	avengModel = std::make_unique<AvengModel>(engineDevice, vertices);
	//}

	/*
	*/
	void XOne::loadAppObjects() 
	{
		std::vector<AvengModel::Vertex> vertices { // vector
			{ {0.0f, -0.5f }, {1.0f, 0.0f, 0.0f} }, // Model Vertex
			{ {0.5f,  0.5f }, {0.0f, 1.0f, 0.0f} },
			{ {-0.5f, 0.5f }, {0.0f, 0.0f, 1.0f} }
		};
		auto avengModel = std::make_shared<AvengModel>(engineDevice, vertices);

		std::vector<glm::vec3> colors{
			{1.f, .9f, .9f},
			{1.f, .2f, .53f},
			{.8f, 1.f, .43f},
			{.2f, 1.f, .8f},
			{.3f, .88f, 1.f}
		};

		for (auto& color : colors) {
			color = glm::pow(color, glm::vec3{ 2.2f });
		}

		for (uint8_t i = 1; i < 100; i++) {

			// By using a shared ptr here we are making sure that 1 model instance can be used by multiple AppObjects
			// It will stay in memory so long as 1 object is still using it
			

			auto triangle = AvengAppObject::createAppObject();
			triangle.model = avengModel;
			triangle.color = {i % 255, i % 255, i % 255};
			//triangle.transform2d.translation.x = .2f;
			//triangle.transform2d.scale = { .5f, 2.f };
			triangle.transform2d.rotation = i * glm::two_pi<float>() * .025f;
			triangle.color = colors[i % colors.size()];
			appObjects.push_back(std::move(triangle));

		}


	}

	/*
	*/
	void XOne::createPipeline()
	{
		// Initialize the pipeline configuration
		assert(lveSwapChain != nullptr && "Cannot create pipeline before swap chain");
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfig pipelineConfig{};
		GFXPipeline::defaultPipelineConfig(pipelineConfig);
		pipelineConfig.renderPass = aveng_swapchain->getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		gfxPipeline = std::make_unique<GFXPipeline>(
			engineDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
		);
	}


	void XOne::recreateSwapChain()
	{
		// Gety current window size
		auto extent = aveng_window.getExtent();

		// If the program has at least 1 dimension of 0 size (it's minimized); wait
		while (extent.width == 0 || extent.height == 0)
		{
			extent = aveng_window.getExtent();
			glfwWaitEvents(); 
		}

		// Wait until the current swap chain isn't being used before we attempt to construct the next one.
		vkDeviceWaitIdle(engineDevice.device());

		aveng_swapchain = nullptr;

		if (aveng_swapchain == nullptr) {
			// Create the new swapchain object
			aveng_swapchain = std::make_unique<SwapChain>(engineDevice, extent);
			
		}
		else {
			// 
			aveng_swapchain = std::make_unique<SwapChain>(engineDevice, extent, std::move(aveng_swapchain));
			if (aveng_swapchain->imageCount() != commandBuffers.size())
			{
				freeCommandBuffers();
				createCommandBuffers();
			}
		}

		// The pipeline needs to be recreated || TODO - but not if the render pass is already compatible
		createPipeline();

	}


	void XOne::freeCommandBuffers()
	{
		vkFreeCommandBuffers(
			engineDevice.device(), 
			engineDevice.getCommandPool(), 
			static_cast<uint32_t>(commandBuffers.size()), 
			commandBuffers.data()
		);

		commandBuffers.clear();
	}


	/*
	*/
	void XOne::createCommandBuffers() {
	
		commandBuffers.resize(aveng_swapchain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		// Command buffer memory is allocated from a command pool
		allocInfo.commandPool = engineDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(engineDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers.");
		}

	}
	void XOne::recordCommandBuffer(int imageIndex)
	{
	
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
		{
			std::runtime_error("Command Buffer failed to begin recording.");
		}
		/*
			Record Commands
		*/

		// 1. Begin a render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = aveng_swapchain->getRenderPass();
		renderPassInfo.framebuffer = aveng_swapchain->getFrameBuffer(imageIndex);

		// The area where shader loading and storing takes place.
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = aveng_swapchain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// 2. Submit to command buffers to begin the render pass

		// VK_SUBPASS_CONTENTS_INLINE signals that subsequent renderpass commands come directly from the primary command buffer.
		// No secondary buffers are currently being utilized.
		// For this reason we cannot Mix both Inline command buffers AND secondary command buffers in our render pass execution.
		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Configure the viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(aveng_swapchain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(aveng_swapchain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, aveng_swapchain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

		renderAppObjects(commandBuffers[imageIndex]);

		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer.");
		}
	}

	void XOne::renderAppObjects(VkCommandBuffer commandBuffer)
	{
		gfxPipeline->bind(commandBuffer);

		for (auto& obj : appObjects) {
			// Rotation
			obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.01f, glm::two_pi<float>());

			SimplePushConstantData push{};
			push.offset = obj.transform2d.translation;
			push.color = obj.color;
			push.transform = obj.transform2d.mat2();

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}

	/*
	* In this function we'll not only draw the frame but we'll decide if
	* the swapchain needs to be recreated prior to doing so
	*/
	void XOne:: drawFrame() {

		uint32_t imageIndex;

		auto result = aveng_swapchain->acquireNextImage(&imageIndex);

		// This error will occur after window resize
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		// This could potentially occur during window resize events
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image.");
		}

		recordCommandBuffer(imageIndex);

		// Submit to graphics queue while handlind cpu and gpu sync, executing the command buffers
		result = aveng_swapchain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || aveng_window.wasWindowResized())
		{
			aveng_window.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image.");
		}

		// The command buffer will now be executed

	}


} //