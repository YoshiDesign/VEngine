#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace aveng {

	// All kinds of matrix
	struct TransformComponent
	{
		glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.f, 1.f, 1.f };
		float modPI = 3.14159;

		// Matrix corresponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 _mat4();

		glm::mat3 normalMatrix();

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& _translation)
			: translation(_translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(rotation));

			return glm::translate(glm::mat4(1.0f), translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), scale);
		}

	};

	struct VisualComponent {
		int tex_id;
	};

	struct MotionComponent {
		float speed;
	};

	struct MetaComponent{
		int type;
	};
	
	struct RigidBodyComponent
	{
		glm::vec2 velocity;
		float mass{ 1.0f };
	};

}