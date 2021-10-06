#pragma once

#include <memory>
#include <vector>

#include "Core/Renderer/RenderSystem.h"
#include "CoreVK/aveng_descriptors.h"
#include "Core/Renderer/AvengImageSystem.h"
#include "Core/app_object.h"
#include "Core/Peripheral/aveng_window.h"
#include "CoreVK/EngineDevice.h"
#include "CoreVk/aveng_buffer.h"
#include "Core/Renderer/Renderer.h"
#include "Core/Peripheral/KeyboardController.h"

namespace aveng {

	class XOne {

	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		struct GlobalUbo {
			alignas(16) glm::mat4 projectionView{ 1.f };
			alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{ -1.f, -3.f, 1.f });
		};

		XOne();
		~XOne();

		XOne(const XOne&) = delete;
		XOne &operator=(const XOne&) = delete;
		// int fib(int n, int a = 0, int b = 1);
		
		void run();
		void updateData();

	private:

		void loadAppObjects();
		void updateCamera(float frameTime, AvengAppObject& viewerObject, KeyboardController& cameraController, AvengCamera& camera);

		// The window API - Stack allocated
		AvengWindow aveng_window{ WIDTH, HEIGHT, "Vulkan 0" };
		glm::vec3 clear_color = { 0.0f, 0.0f, 0.0f };

		// !! Order of initialization matters !!
		EngineDevice engineDevice{ aveng_window };
		ImageSystem imageSystem{ engineDevice };
		Renderer renderer{ aveng_window, engineDevice };
		KeyboardController cameraController{};
		AvengCamera camera{};
		GlobalUbo ubo{};

		AvengAppObject viewerObject = AvengAppObject::createAppObject(1000);

		// 
		float aspect;
		float frameTime;
		Data data;

		// This declaration must occur after the renderer initializes
		std::unique_ptr<AvengDescriptorPool> globalPool{};
		std::vector<AvengAppObject> appObjects;

		// Band-aid when all seems lost
		//auto minOffsetAlignment = std::lcm(
		//	engineDevice.properties.limits.minUniformBufferOffsetAlignment,
		//	engineDevice.properties.limits.nonCoherentAtomSize);

	};

}
