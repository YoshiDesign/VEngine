#include "ObjectRenderSystem.h"
#include "../Math/aveng_math.h"
#include "../Events/window_callbacks.h"
#include "../Player/GameplayFunctions.h"

namespace aveng {

	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	ObjectRenderSystem::ObjectRenderSystem(EngineDevice& device, AvengAppObject& viewer)
		: engineDevice{ device }, viewerObject{ viewer }
	{

	}

	void ObjectRenderSystem::initialize( VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout, VkDescriptorSetLayout objDescriptorSetLayout)
	{
		VkDescriptorSetLayout descriptorSetLayouts[2] = { globalDescriptorSetLayout , objDescriptorSetLayout };
		createPipelineLayout(descriptorSetLayouts);
		createPipeline(renderPass);
	}

	ObjectRenderSystem::~ObjectRenderSystem()
	{
		vkDestroyPipelineLayout(engineDevice.device(), pipelineLayout, nullptr);
	}

	/*
	 * Setup of the pipeline layout. 
	 * Here we include our Push Constant information
	 * as well as our descriptor set layouts.
	 */
	void ObjectRenderSystem::createPipelineLayout(VkDescriptorSetLayout* descriptorSetLayouts)
	{

		// Initialize push constant range(s)
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);	// Must be a multiple of 4

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2;										// How many descriptor set layouts are to be hooked into the pipeline
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;						// a pointer to an array of VkDescriptorSetLayout objects.
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		// Create the pipeline layout, updating our pipelineLayout member.
		if (vkCreatePipelineLayout(engineDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

	}

	/*
	* Call to the construction of a Graphics Pipeline.
	* Note that shader filepaths are relative to the GFXPipeline.cpp file.
	*/
	void ObjectRenderSystem::createPipeline(VkRenderPass renderPass)
	{
		// Initialize the pipeline 
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfig pipelineConfig{};
		GFXPipeline::defaultPipelineConfig(pipelineConfig);
		pipelineConfig.renderPass = renderPass;		
		pipelineConfig.pipelineLayout = pipelineLayout;

		// A GFXPipeline
		gfxPipeline = std::make_unique<GFXPipeline>(
			engineDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
		);

		// Another GFXPipeline
		gfxPipeline2 = std::make_unique<GFXPipeline>(
			engineDevice,
			"shaders/simple_shader2.vert.spv",
			"shaders/simple_shader2.frag.spv",
			pipelineConfig
		);
	}

	void ObjectRenderSystem::render(FrameContent& frame_content, Data& data, AvengBuffer& u_ObjBuffer)
	{

		// 1s tick, convenient
		if (last_sec != data.sec) {
			last_sec = data.sec;
			std::cout << "Tick..." << std::endl;
		}

		// Bind our current pipeline configuration
		switch (data.cur_pipe)
		{
			case 98: gfxPipeline->bind(frame_content.commandBuffer);  break;
			case 99: gfxPipeline2->bind(frame_content.commandBuffer); break;
			default:
				gfxPipeline->bind(frame_content.commandBuffer); // 0
		}

		vkCmdBindDescriptorSets(
			frame_content.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frame_content.globalDescriptorSet,
			0,
			nullptr);

		updateData(frame_content.appObjects.size(), frame_content.frameTime, data);

		/*
		* Thread object bind/draw calls here
		*/
		int i = 0;
		for (auto& kv : frame_content.appObjects)
		{
			i++;
			ObjectUniformData u_ObjData{ kv.second.get_texture() };	// Contains texture index

			// Push Constant Data
			SimplePushConstantData push{};
			push.modelMatrix  = kv.second.transform._mat4();
			push.normalMatrix = kv.second.transform.normalMatrix();

			uint32_t dynamicOffset = engineDevice.properties.limits.minUniformBufferOffsetAlignment * i;
			if (dynamicOffset > engineDevice.properties.limits.maxUniformBufferRange) {
			    //This will (should) never occur because we're taking measurments from the same source when instantiating the dynamic uniform buffer
				// Remove this for any release builds
				DEBUG("Max Uniform Buffer Range Exceeded.");
				throw std::runtime_error("Attempting to allocate buffer beyond device uniform buffer memory limit.");
			}

			// Bind the descriptor set for our pixel (fragment) shader
			u_ObjBuffer.writeToBuffer(&u_ObjData, sizeof(ObjectUniformData), dynamicOffset);
			u_ObjBuffer.flush();

			vkCmdBindDescriptorSets(
				frame_content.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,
				1,
				&frame_content.objectDescriptorSet,
				1,
				&dynamicOffset);

			vkCmdPushConstants(
				frame_content.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);

			kv.second.model->bind(frame_content.commandBuffer);
			kv.second.model->draw(frame_content.commandBuffer);

		}
	}

	void ObjectRenderSystem::updateData(size_t size, float frameTime, Data& data)
	{

		data.num_objs = size;
		data.cur_pipe = WindowCallbacks::getCurPipeline();
		data.dt = frameTime;
		data.camera_modPI = viewerObject.transform.modPI;

	}

} //