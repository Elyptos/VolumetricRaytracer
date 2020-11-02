#pragma once
#include <memory>
#include "Object.h"

namespace VolumeRaytracer
{
	namespace UI
	{
		class VWindow;

		class VWindowFactory
		{
		public:
			static VObjectPtr<VWindow> NewWindow();
		};
	}
}