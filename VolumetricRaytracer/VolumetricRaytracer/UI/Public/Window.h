#pragma once

#include <string>
#include "Object.h"

namespace VolumeRaytracer
{
	namespace UI
	{
		class VWindow : public VObject
		{
		public:
			VWindow();
			virtual ~VWindow();

			virtual void SetTitle(const std::wstring& title) = 0;
			virtual void SetSize(const unsigned int& width, const unsigned int& height) = 0;

			virtual unsigned int GetWidth() const = 0;
			virtual unsigned int GetHeight() const = 0;

		protected:
			virtual void InitializeWindow() = 0;
			virtual void CloseWindow() = 0;

			void Initialize() override;
			void BeginDestroy() override;

			bool CanEverTick() const override;
			bool ShouldTick() const override;
		};
	}
}