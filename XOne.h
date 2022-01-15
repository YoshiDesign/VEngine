#pragma once

#include <memory>
#include <vector>

#include "Core/Renderer/RenderSystem.h"
#include "CoreVK/aveng_descriptors.h"
#include "Core/Renderer/AvengImageSystem.h"
#include "Core/Renderer/PointLightSystem.h"
#include "Core/app_object.h"
#include "GUI/aveng_imgui.h"
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
			glm::mat4 projection{ 1.f };
			glm::mat4 view{ 1.f };
			glm::vec4 ambientLightColor{0.f, 0.f, 1.f, .04f};
			glm::vec3 lightPosition{ 5.0f, -1.0f, 2.8f };
			alignas(16) glm::vec4 lightColor{ 1.f, 1.f, 1.f, 1.f };
			//alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{ -1.f, -3.f, 1.f });
		};

		XOne();
		~XOne();

		XOne(const XOne&) = delete;
		XOne &operator=(const XOne&) = delete;
		// int fib(int n, int a = 0, int b = 1);
		
		void run();

		void pendulum(EngineDevice& engineDevice, int _max_rows);

	private:

		void loadAppObjects();
		void updateCamera(float frameTime, AvengAppObject& viewerObject, KeyboardController& cameraController, AvengCamera& camera);
		void updateData();

		// The window API - Stack allocated
		AvengWindow aveng_window{ WIDTH, HEIGHT, "Vulkan 0" };
		glm::vec3 clear_color = { 0.0f, 0.0f, 0.0f };

		// !! Order of initialization matters !!
		Data data;
		EngineDevice engineDevice{ aveng_window };
		ImageSystem imageSystem{ engineDevice };
		Renderer renderer{ aveng_window, engineDevice };
		AvengImgui aveng_imgui{ engineDevice };
		AvengCamera camera{};
		GlobalUbo ubo{};

		AvengAppObject viewerObject{ AvengAppObject::createAppObject(1000) };
		// AvengAppObject playerObject{ AvengAppObject::createAppObject(THEME_3) };

		// 
		float aspect;
		float frameTime;

		// This declaration must occur after the renderer initializes
		std::unique_ptr<AvengDescriptorPool> globalPool{};
		AvengAppObject::Map appObjects;

		// Band-aid when all seems lost
		//auto minOffsetAlignment = std::lcm(
		//	engineDevice.properties.limits.minUniformBufferOffsetAlignment,
		//	engineDevice.properties.limits.nonCoherentAtomSize);

	};

}
