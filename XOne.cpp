#include "Camera/aveng_camera.h"
#include "KeyControl/KeyboardController.h"
#include "aveng_imgui.h"
#include "aveng_buffer.h"
#include "aveng_textures.h"
#include "XOne.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
#include <array>
#include <numeric>
#include <chrono>

#define LOG(a) std::cout<<a<<std::endl;

namespace aveng {

	// For use similar to a push_constant struct. Passing in read-only data to the pipeline shader modules
	struct GlobalUbo {
		alignas(16) glm::mat4 projectionView{ 1.f };
		alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -1.f, 1.f });

	};

	int current_pipeline = 0;
	void testKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			current_pipeline = (current_pipeline + 1) % 2;
		}
	}

	XOne::XOne() 
	{
		globalPool = AvengDescriptorPool::Builder(engineDevice)
			.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();


		loadAppObjects();
	}

	XOne::~XOne()
	{
		
	}

	void XOne::run()
	{
		glfwSetKeyCallback(aveng_window.getGLFWwindow(), testKeyCallback);

		auto minOffsetAlignment = std::lcm(
			engineDevice.properties.limits.minUniformBufferOffsetAlignment,
			engineDevice.properties.limits.nonCoherentAtomSize);

		// Creating a uniform buffer
		AvengBuffer globalUboBuffer{
			engineDevice,
			sizeof(GlobalUbo),
			SwapChain::MAX_FRAMES_IN_FLIGHT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			minOffsetAlignment
		};

		// enable writing to it's memory
		globalUboBuffer.map();

		auto globalSetLayout =
			AvengDescriptorSetLayout::Builder(engineDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = globalUboBuffer.descriptorInfoForIndex(i);
			AvengDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		// Note that the renderSystem is initialized with a pointer to the Render Pass
		RenderSystem renderSystem{ engineDevice, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		AvengCamera camera{};

		//camera.setViewTarget(glm::vec3(-1.f, -2.f, -20.f), glm::vec3(0.f, 0.f, 3.5f));

		// Has no model or rendering. Used to store the camera's current state
		auto viewerObject = AvengAppObject::createAppObject();

		KeyboardController cameraController{};

		AvengImgui aveng_imgui{
			aveng_window,
			engineDevice,
			renderer.getSwapChainRenderPass(),
			renderer.getImageCount() 
		};

		/*
			Things to keep in mind:
			Object Space - Objects initially exist at the origin of object space
			World Space  - The model matrix created by the AppObject's transform component coordinates objects with World Space
			Camera Space - The view transformation, applied to our objects, moves objects from World Space into Camera Space;
							where the camera is at the origin and all object's coord's are relative to their position and orientation

					* The camera does not actually exist, we're just transforming objects AS IF the camera were there
					
				We then apply the projection matrix, capturing whatever is contained by the viewing frustrum, which then transforms
				it to the canonical view volume. As a final step the viewport transformation maps this region to actual pixel values.
					
		*/

		auto currentTime = std::chrono::high_resolution_clock::now();

		// Keep the window open until shouldClose is truthy
		while (!aveng_window.shouldClose()) {

			// Keep this on top. It can block
			glfwPollEvents();

			// Maintain the aspect ratio
			float aspect = renderer.getAspectRatio();
			// Calculate time between iterations
			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			//frameTime = glm::min(frameTime, MAX_FRAME_TIME);	// Use this to lock to a specific max frame rate

			// Updates the viewer object transform component based on key input, proportional to the time elapsed since the last frame
			cameraController.moveInPlaneXZ(aveng_window.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
			auto commandBuffer = renderer.beginFrame();

			if (commandBuffer != nullptr) {

				int frameIndex = renderer.getFrameIndex();
				FrameContent frame_content{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex]
				};

				// Update our global uniform buffer
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				globalUboBuffer.writeToIndex(&ubo, frameIndex);
				globalUboBuffer.flushIndex(frameIndex);

				// Render
				renderer.beginSwapChainRenderPass(commandBuffer);
				renderSystem.renderAppObjects(frame_content, appObjects, current_pipeline );
				aveng_imgui.newFrame();
				aveng_imgui.runGUI(appObjects.size());
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
		
		std::shared_ptr<AvengModel> avengModel = AvengModel::createModelFromFile(engineDevice, "C:/dev/3DModels/cube.obj");

		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 100; j++) {
				for (int k = 0; k < 100; k++){

					auto gameObj = AvengAppObject::createAppObject();

					gameObj.model = avengModel;
					gameObj.transform.translation = { static_cast<float>(i) * 1.02f, static_cast<float>(k) * 1.02, static_cast<float>(j) * 1.05f };
					gameObj.transform.scale = { .05f, .05f, .05f };

					appObjects.push_back(std::move(gameObj));

				}

			}
		
		}
		//auto gameObj = AvengAppObject::createAppObject();
		//gameObj.model = avengModel;
		//gameObj.transform.translation = { 3.0f , 0.0f, 3.3f};
		//gameObj.transform.scale = { 1.0f, 1.0f, 1.0f };

		//appObjects.push_back(std::move(gameObj));
	}

} // ns aveng