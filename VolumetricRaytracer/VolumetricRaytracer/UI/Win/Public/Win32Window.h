#pragma once

#include "Window.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace VolumeRaytracer
{
	namespace UI
	{
		namespace Win32
		{
			class VWin32Window : public VWindow
			{
			public:
				VWin32Window();
				~VWin32Window();

				void SetTitle(const std::wstring& title) override;
				void SetSize(const unsigned int& width, const unsigned int& height) override;

				unsigned int GetWidth() const override;
				unsigned int GetHeight() const override;

			protected:
				void InitializeWindow() override;
				void CloseWindow() override;


				void Tick(const float& deltaSeconds) override;

			private:
				uint32_t Width;
				uint32_t Height;

				HINSTANCE HInstance;
				HWND WindowHandle;
			};
		}
	}
}