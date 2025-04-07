#include "avpch.h"
#include "XOne.h"
#include "Core/Math/aveng_math.h"
#include "Core/data.h"
#include "Core/Utils/aveng_utils.h"
#include "Core/aveng_frame_content.h"
#include "Core/Camera/aveng_camera.h"
#include "Core/Events/window_callbacks.h"
#include "Core/Player/GameplayFunctions.h"

namespace aveng {

	// Dynamic Helpers on window callback keys
	int WindowCallbacks::current_pipeline{ 1 };
	glm::vec3 WindowCallbacks::modRot{ 0.0f, 0.0f, 0.0f };
	glm::vec3 WindowCallbacks::modTrans{ 0.0f, 0.0f, 0.0f };
	int WindowCallbacks::posNeg = 1;
	bool WindowCallbacks::flightMode = false;
	float WindowCallbacks::modPI = PI;

	XOne::XOne() 
	{
		loadAppObjects();
		Setup();
		
	}

	void XOne::run()
	{
		// Set callback functions for keys bound to the window
		glfwSetKeyCallback(aveng_window.getGLFWwindow(), WindowCallbacks::testKeyCallback);

		//camera.setViewTarget(glm::vec3(-1.f, -2.f, -20.f), glm::vec3(0.f, 0.f, 3.5f));

		auto currentTime = std::chrono::high_resolution_clock::now();

		// Initial camera position
		viewerObject.transform.translation.z = -5.5f;
		viewerObject.transform.translation.y = -2.5f;

		// Render Loop
		while (!aveng_window.shouldClose()) {

			// Potentially blocking
			glfwPollEvents();

			// Calculate time between iterations
			auto newTime = std::chrono::high_resolution_clock::now();
			frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			// Data & Debug
			updateCamera(frameTime, viewerObject, keyboardController, camera);
			updateData();

			// Get a command buffer for this frame
			VkCommandBuffer commandBuffer = renderer.beginFrame();

			if (commandBuffer != nullptr) {

				int frameIndex = renderer.getFrameIndex();

				FrameContent frame_content = {
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					objectDescriptorSets[frameIndex],
					appObjects
				};

				// Pack our vertex shader uniform buffer
				u_GlobalData.projection = camera.getProjection();
				u_GlobalData.view = camera.getView();

				// Update our global uniform buffer 
				u_GlobalBuffers[frameIndex]->writeToBuffer(&u_GlobalData);
				u_GlobalBuffers[frameIndex]->flush();

				// Render
				renderer.beginSwapChainRenderPass(commandBuffer);

				objectRenderSystem.render(frame_content, data, *u_ObjBuffers[frameIndex]);
				pointLightSystem.render(frame_content);

				aveng_imgui.newFrame();
				aveng_imgui.runGUI(data);
				aveng_imgui.render(commandBuffer);

				renderer.endSwapChainRenderPass(commandBuffer);
				renderer.endFrame();
				
			}

		}

		// Block until all GPU operations quit.
		vkDeviceWaitIdle(engineDevice.device());
	}

	/*
	* 
	*/
	void XOne::loadAppObjects() 
	{

		auto ship = AvengAppObject::createAppObject(THEME_1);
		ship.model = AvengModel::createModelFromFile(engineDevice, "3D/ship.obj");
		ship.transform.translation = { 0.f, 0.f, 0.f };
		appObjects.emplace(ship.getId(), std::move(ship));

		auto ship_1 = AvengAppObject::createAppObject(THEME_3);
		ship_1.model = AvengModel::createModelFromFile(engineDevice, "3D/ship.obj");
		ship_1.transform.translation = { 25.f, 0.f, 0.f };
		appObjects.emplace(ship_1.getId(), std::move(ship_1));

		// AvengModel::drawTriangle(engineDevice, { 1.0f, 1.0f, 1.0f });



		//for (size_t i = 0; i < 1; i++)
		//{

		//	for (size_t j = 0; j < 1; j++)
		//	{

		//		auto grid = AvengAppObject::createAppObject(THEME_2);
		//		grid.meta.type = GROUND;
		//		grid.model = AvengModel::createModelFromFile(engineDevice, "3D/plane.obj");
		//		grid.transform.translation = { 136.0f * i, -.1f, 0.0f};
		//		appObjects.emplace(grid.getId(), std::move(grid));

				//auto grid2 = AvengAppObject::createAppObject(THEME_1);
				//grid2.meta.type = GROUND;
				//grid2.model = AvengModel::createModelFromFile(engineDevice, "3D/plane.obj");
				//grid2.transform.translation = { 150.0f, -.1f, 170.0f };
				//appObjects.emplace(grid2.getId(), std::move(grid2));

		//	}

		//}

		//for (size_t i = 0; i < 10; i++) {
		//	for (size_t j = 0; j < 15; j++) {
		//		for (size_t k = 0; k < 5; k++) {

		//			auto triangle = AvengAppObject::createAppObject(NO_TEXTURE);
		//			triangle.meta.type = SCENE;
		//			triangle.model = AvengModel::drawTriangle(engineDevice, {static_cast<float>(i), static_cast<float>(j) * -1.0f, static_cast<float>(k) });
		//			appObjects.push_back(std::move(triangle));

		//		}
		//	}
		//}

		/*for (size_t i = 0; i < 10; i++)
		{
			
			for (size_t j = 0; j < 50; j++) 
			{

				for (size_t k = 0; k < 4; k++) {
					auto sphere = AvengAppObject::createAppObject(NO_TEXTURE);
					sphere.meta.type = SCENE;
					sphere.model = AvengModel::createModelFromFile(engineDevice, "3D/sphere.obj");
					sphere.transform.translation = { static_cast<float>(i) * 1.5f, static_cast<float>(j) * -1.0f, static_cast<float>(k) * 2.0f };
					sphere.transform.scale = {0.1f, 0.1f, 0.1f};
					appObjects.emplace(sphere.getId(), std::move(sphere));
				
				}
			
			}

		}*/

	}

	void XOne::updateCamera(float frameTime, AvengAppObject& viewerObject, KeyboardController& keyboardController, AvengCamera& camera)
	{
		aspect = renderer.getAspectRatio();
		// Updates the viewer object transform component based on key input, proportional to the time elapsed since the last frame
		keyboardController.moveCameraXZ(aveng_window.getGLFWwindow(), frameTime);
		camera.setViewYXZ(viewerObject.transform.translation + glm::vec3(0.f, 0.f, -.80f), viewerObject.transform.rotation + glm::vec3());
		camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
	}

	void XOne::updateData()
	{
		data.cameraView = camera.getCameraView();
		data.cameraPos  = viewerObject.transform.translation;
		data.cameraRot  = viewerObject.transform.rotation;
		data.fly_mode   = WindowCallbacks::flightMode;
	}

	/*
	* @function XOne::Setup()
	* Write the descriptor set layouts, build the descriptor sets for our uniforms,
	* and initialize each render system with their appropriate descriptor set layouts.
	* Finally, initialize ImGUI
	*/
	void XOne::Setup()
	{
		std::cout << "Initializing App Setup..." << std::endl;
		std::cout << "MinUniformbufferOffsetAlignment" << engineDevice.properties.limits.minUniformBufferOffsetAlignment
			<< "\nSize of Dynamic Uniform Buffer:\t" <<
			sizeof(ObjectRenderSystem::ObjectUniformData)
			* engineDevice.properties.limits.minUniformBufferOffsetAlignment
			* appObjects.size() << " Bytes"
			<< "\nSize of ObjectUniformBuffer Data\t" << sizeof(ObjectRenderSystem::ObjectUniformData)
			<< std::endl;

		VkPhysicalDeviceFeatures m;
		vkGetPhysicalDeviceFeatures(engineDevice.physicalDevice(), &m);
		if (!m.shaderSampledImageArrayDynamicIndexing) {
			// If shaderSampledImageArrayDynamicIndexing == 0 the shaders/hardware do not support image samplers
			// This is required to load any sort of texture
			std::runtime_error("Your hardware does not support this specific VulkanAPI implementation. Sorry!");
		}

		/*
		 * Call the pool builder to setup our pool for construction.
		 */
		descriptorPool = AvengDescriptorPool::Builder(engineDevice)
			.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * 4)
						 // Type									// Max no. of descriptor sets
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * 8)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
			.build();

		// Create uniform buffers mapped into device memory
		u_GlobalBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
		u_ObjBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < u_GlobalBuffers.size(); i++) {
			u_GlobalBuffers[i] = std::make_unique<AvengBuffer>(engineDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			u_GlobalBuffers[i]->map();
		}

		if (sizeof(ObjectRenderSystem::ObjectUniformData) > engineDevice.properties.limits.minUniformBufferOffsetAlignment)
		{
			// We'll need to update our alignment should this ever be the case.
			throw std::runtime_error("ObjectUniformData is larger than the minimum offset alignment. Uniform Data size needs to be updated.");
		}

		for (int i = 0; i < u_ObjBuffers.size(); i++) {
			u_ObjBuffers[i] = std::make_unique<AvengBuffer>(engineDevice,
				sizeof(ObjectRenderSystem::ObjectUniformData) * engineDevice.properties.limits.minUniformBufferOffsetAlignment * appObjects.size(), // The size <VkDeviceSize> of the dynamic uniform buffer 
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				engineDevice.properties.limits.minUniformBufferOffsetAlignment);
			u_ObjBuffers[i]->map();
		}

		std::cout << "XOne -- Creating global Descriptors" << std::endl;
		// Descriptor Layout 0 -- Global
		std::unique_ptr<AvengDescriptorSetLayout> globalDescriptorSetLayout =
			AvengDescriptorSetLayout::Builder(engineDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, imageSystem.descriptorInfoForAllImages().size())	// Combined image samplers use 1 descriptor for each image
			.build();

		std::cout << "XOne -- Creating obj Descriptors" << std::endl;
		// Descriptor Set 1 -- Per object
		std::unique_ptr<AvengDescriptorSetLayout> objDescriptorSetLayout =
			AvengDescriptorSetLayout::Builder(engineDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
			.build();

		// Write our descriptors according to the layout's bindings once for each frame in flight
		globalDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
		objectDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		// Create the descriptor sets, once for each swapchain frame
		for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
		{
			// Write first set - Uniform Buffer containing our global-UBO and our Imager Sampler
			auto bufferInfo = u_GlobalBuffers[i]->descriptorInfo(sizeof(GlobalUbo), 0);
			auto imageInfo = imageSystem.descriptorInfoForAllImages();
			std::cout << "Writing Global DescriptorSet" << std::endl;
			AvengDescriptorSetWriter(*globalDescriptorSetLayout, *descriptorPool)
				.writeBuffer(0, &bufferInfo)	// First Binding descriptor: Buffer
				.writeImage(1, imageInfo.data(), imageInfo.size()) // Second Binding descriptor: Image
				.build(globalDescriptorSets[i]);

			std::cout << "Writing Object DescriptorSet" << std::endl;
			// Write second set - Also a uniform buffer
			auto objBufferInfo = u_ObjBuffers[i]->descriptorInfo(sizeof(ObjectRenderSystem::ObjectUniformData), 0);
			AvengDescriptorSetWriter(*objDescriptorSetLayout, *descriptorPool)
				.writeBuffer(0, &objBufferInfo)
				.build(objectDescriptorSets[i]);
		}

		// Rendering subsystem initializers
		objectRenderSystem.initialize(
			renderer.getSwapChainRenderPass(),
			globalDescriptorSetLayout->getDescriptorSetLayout(),
			objDescriptorSetLayout->getDescriptorSetLayout()
		);
		pointLightSystem.initialize(
			renderer.getSwapChainRenderPass(),
			globalDescriptorSetLayout->getDescriptorSetLayout()
		);
		// GUI
		aveng_imgui.init(
			aveng_window,
			renderer.getSwapChainRenderPass(),
			renderer.getImageCount()
		);
	}

	void XOne::pendulum(EngineDevice& engineDevice, int _max_rows)
	{

		std::vector<float> factors;
		float length;
		float time = 7.0f;
		float gravity = 3.45f;
		float k = 7.0f;
		int max_rows = _max_rows;
		int row_modifier = 0;

		//std::unique_ptr<AvengModel> coloredCubeModel = AvengModel::createModelFromFile(engineDevice, "3D/colored_cube.obj");

		for (size_t i = 0; i < max_rows; i++)
		{
			//row_modifier = row_modifier % static_cast<int>(ceil(max_rows / 2) + 1);
			for (size_t j = 0; j < 1; j++) {
				auto gameObj = AvengAppObject::createAppObject(1000);
				//gameObj.model = coloredCubeModel;
				gameObj.model = AvengModel::createModelFromFile(engineDevice, "3D/colored_cube.obj");
				gameObj.meta.type = SCENE;

				if (i >= std::floor(max_rows / 2))
					gameObj.visual.pendulum_row = max_rows - row_modifier;
				else
					gameObj.visual.pendulum_row = row_modifier;

				length = gravity * glm::pow((time / (2 * glm::pi<float>()) * (k + gameObj.visual.pendulum_row + 1)), 2);
				length = length * .003;

				gameObj.visual.pendulum_delta = 0.0f;
				// To make this an actual pendulum, make the extent constant across all objects
				gameObj.visual.pendulum_extent = 70;
				gameObj.transform.velocity.x = length;
				gameObj.transform.translation = { 0.0f, static_cast<float>((i * -1.0f)), 0.0f };
				gameObj.transform.scale = { .4f, 0.4f, 0.4f };

				appObjects.emplace(gameObj.getId(), std::move(gameObj));
			}
			row_modifier++;
		}

	}

}