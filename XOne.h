#pragma once

#include <memory>
#include <vector>

#include "Renderer/RenderSystem.h"
#include "aveng_descriptors.h"
#include "Renderer/AvengImageSystem.h"
#include "app_objects/app_object.h"
#include "aveng_window.h"
#include "EngineDevice.h"
#include "Renderer/Renderer.h"
#include "KeyControl/KeyboardController.h"
#include "Utils/data.h"

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

	private:

		void loadAppObjects();
		void updateCamera(float frameTime, AvengAppObject& viewerObject, float aspect, KeyboardController& cameraController, AvengCamera& camera);

		// The window API - Stack allocated
		AvengWindow aveng_window{ WIDTH, HEIGHT, "Vulkan 0" };
		glm::vec4 mods;
		glm::vec3 clear_color = { 0.0f, 0.0f, 0.0f };

		// !! Order of initialization matters !!
		EngineDevice engineDevice{ aveng_window };
		Renderer renderer{ aveng_window, engineDevice };
		AvengCamera camera{};

		// This declaration must occur after the renderer initializes
		std::unique_ptr<AvengDescriptorPool> globalPool{};
		std::vector<AvengAppObject> appObjects;
		
	};

}
