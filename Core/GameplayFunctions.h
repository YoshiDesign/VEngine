#pragma once
#include "app_object.h"
#include "Peripheral/KeyboardController.h"

namespace aveng {

	class GameplayFunctions {

	public:

		static float torque;
		static float deltaX;
		static float deltaZ;
		static float e_out;
		static float clamp_speed;
		static bool  clamped;
		static float _nudge;
		static float camera_deltaX;

		struct init {

		};

		GameplayFunctions()  = delete;

		static glm::vec3 player_spawn_point();
		static bool doBarrelRoll(AvengAppObject& appObject, float dt, int dir);
		static float counterTorque(float dt, float torque);
		//static float centerCorrection(float dt, float x, float vx);
		static float recenterObject(float dt, float x, float vx);
		static float nudge(int direction, float dt);
		static float dx_high(float dt, float rate);
		static float dx_low(float dt, float rate);
		static float camera_dx_high(float dt, float rate);
		static float camera_dx_low(float dt, float rate);
		static float dz_high(float dt);
		static float dz_low(float dt);

	private:

	};

}