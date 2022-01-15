#pragma once

#include "../app_object.h"
#include "../aveng_frame_content.h"
#include "../Peripheral/KeyboardController.h"
#include "../../CoreVK/EngineDevice.h"
#include "../../CoreVK/GFXPipeline.h"
#include "../data.h"

#include "../../avpch.h"

namespace aveng {

	class RenderSystem {

	public:

		struct FragUbo {
			alignas(sizeof(int)) int imDex;
		};

		RenderSystem(EngineDevice &device, AvengAppObject& viewer, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout, VkDescriptorSetLayout fragDescriptorSetLayouts);
		~RenderSystem();

		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator=(const RenderSystem&) = delete;
		void renderAppObjects(FrameContent& frame_content, Data& data, AvengBuffer& fragBuffer);
		VkPipelineLayout getPipelineLayout() { return pipelineLayout; }

	private:

		void createPipelineLayout(VkDescriptorSetLayout* descriptorSetLayouts);
		void updateData(size_t size, float frameTime, Data& data);
		void createPipeline(VkRenderPass renderPass);

		int last_sec;
		EngineDevice &engineDevice;
		AvengAppObject& viewerObject;

		size_t deviceAlignment = engineDevice.properties.limits.minUniformBufferOffsetAlignment;

		// Rendering Pipelines - Heap Allocated
		std::unique_ptr<GFXPipeline> gfxPipeline;
		std::unique_ptr<GFXPipeline> gfxPipeline2;
		VkPipelineLayout pipelineLayout;

	};

}