#include <iostream>
#include "Camera/aveng_camera.h"
#include "KeyControl/KeyboardController.h"
#include "aveng_imgui.h"
#include "aveng_buffer.h"
#include "aveng_textures.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
#include <array>
#include <chrono>
#include "XOne.h"

#define LOG(a) std::cout<<a<<std::endl;

namespace aveng {

	// For use similar to a push_constant struct. Passing in read-only data to the pipeline shader modules
	struct GlobalUbo {
		glm::mat4 projectionView{ 1.f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });

	};

	int current_pipeline = 1;
	void testKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			current_pipeline = (current_pipeline + 1) % 2;
		}

		std::cout << "Current Pipeline: " << current_pipeline << std::endl;
	}

	XOne::XOne() 
	{

		loadAppObjects();
	}

	XOne::~XOne()
	{
		
	}

	void XOne::run()
	{

		// Creating a uniform buffer
		AvengBuffer globalUboBuffer{
			engineDevice,
			sizeof(GlobalUbo),
			SwapChain::MAX_FRAMES_IN_FLIGHT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			engineDevice.properties.limits.minUniformBufferOffsetAlignment
		};

		// enable writing to it's memory
		globalUboBuffer.map();

		// Note that the renderSystem is initialized with a pointer to the Render Pass
		RenderSystem renderSystem{ engineDevice, renderer, renderer.getSwapChainRenderPass()};
		AvengCamera camera{};

		camera.setViewTarget(glm::vec3(-1.f, -2.f, -20.f), glm::vec3(0.f, 0.f, 3.5f));

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
			World Space - The model matrix created by the AppObject's transform component brings objects into World Space
			Camera Space - The view transformation, applied to our objects, moves objects from World Space into Camera Space
							where the camera is at the origin and all objects coords are relative to its position and orientation

					* The camera does not actually exist, we're just transforming objects AS IF the camera were there
					
				We then apply the projection matrix, capturing whatever is contained by the viewing frustrum and transforms
				it to the canonical view volume. As a final step the viewport transformation maps this region to actual pixel values.
					
		*/

		auto currentTime = std::chrono::high_resolution_clock::now();

		glfwSetKeyCallback(aveng_window.getGLFWwindow(), testKeyCallback);

		// Keep the window open until shouldClose is truthy
		while (!aveng_window.shouldClose()) {

			// Keep this on top bc it can block
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			// The amount of time which has passed since the last loop iteration
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			//frameTime = glm::min(frameTime, MAX_FRAME_TIME);

			// Updates the viewer object transform component based on key input, proportional to the time elapsed since the last frame
			cameraController.moveInPlaneXZ(aveng_window.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			// Keeps our projection in tune with our window aspect ratio during rendering
			float aspect = renderer.getAspectRatio();

			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

			// The beginFrame function will return a nullptr if the swapchain needs to be recreated
			auto commandBuffer = renderer.beginFrame();

			if (commandBuffer != nullptr) {

				int frameIndex = renderer.getFrameIndex();
				// VkDescriptorSet descriptorSet = descriptorSets[frameIndex];
				// frame relevant data
				FrameContent frame_content{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					renderer.getCurrentDescriptorSet()
				};

				// Update
				/*GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				globalUboBuffer.writeToIndex(&ubo, frameIndex);
				globalUboBuffer.flushIndex(frameIndex);*/
				renderer.updateUniformBuffer(frameIndex, frameTime, camera.getProjection() * camera.getView());

				// Render
				aveng_imgui.newFrame();
				renderer.beginSwapChainRenderPass(commandBuffer);
				renderSystem.renderAppObjects(frame_content, appObjects, current_pipeline );
				aveng_imgui.runGUI(
					appObjects.size()
				);
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
		
		std::shared_ptr<AvengModel> avengModel = AvengModel::createModelFromFile(engineDevice, "C:/dev/3DModels/holy_ship.obj");

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 4; j++) {

				auto gameObj = AvengAppObject::createAppObject();
				
				gameObj.model = avengModel;
				gameObj.transform.translation = { static_cast<float>(i) * 7.0f, 0.0f, static_cast<float>(j) * 5.0f};
				gameObj.transform.scale = { .25f, .25f, .25f };

				appObjects.push_back(std::move(gameObj));

			}
		
		}

	}

} // ns aveng