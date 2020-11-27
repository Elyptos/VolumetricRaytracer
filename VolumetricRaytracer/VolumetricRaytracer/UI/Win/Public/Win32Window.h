/*
	Copyright (c) 2020 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

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

				HWND GetHWND() const;
				HINSTANCE GetHInstance() const;
				LRESULT CALLBACK MessageHandler(HWND hwnd, UINT msg, WPARAM p1, LPARAM p2);

			protected:
				void InitializeWindow() override;
				void CloseWindow() override;


				void Tick(const float& deltaSeconds) override;


				bool AttachToRenderer(Renderer::VRenderer* renderer) override;


				bool DetachFromRenderer(Renderer::VRenderer* renderer) override;

			private:
				uint32_t Width;
				uint32_t Height;

				HINSTANCE HInstance;
				HWND WindowHandle;
			};
		}
	}
}