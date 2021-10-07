#pragma once
#include "app_object.h"

namespace aveng {
	class Command
	{
	public:
		virtual ~Command() {}
		virtual void execute(AvengAppObject& entity, float fTime) = 0;
	};

}