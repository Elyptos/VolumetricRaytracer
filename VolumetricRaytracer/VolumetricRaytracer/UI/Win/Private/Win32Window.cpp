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

#include "Win32Window.h"
#include "Win32Resources.h"
#include <boost/unordered_map.hpp>

namespace VolumeRaytracer
{
	namespace UI
	{
		namespace Win32
		{
			class VWin32MessageDistributor
			{
			public:
				void RegisterWindow(VWin32Window* window)
				{
					RegisteredWindows[window->GetHWND()] = window;
				}

				void UnregisterWindow(VWin32Window* window)
				{
					RegisteredWindows.erase(window->GetHWND());
				}

				boost::unordered_map<HWND, VWin32Window*> RegisteredWindows;
			};

			VWin32MessageDistributor WinMessageDistributor;
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (VolumeRaytracer::UI::Win32::WinMessageDistributor.RegisteredWindows.find(hWnd) != VolumeRaytracer::UI::Win32::WinMessageDistributor.RegisteredWindows.end())
	{
		return VolumeRaytracer::UI::Win32::WinMessageDistributor.RegisteredWindows[hWnd]->MessageHandler(hWnd, message, wParam, lParam);
	}

	return DefWindowProcW(hWnd, message, wParam, lParam);
}

VolumeRaytracer::UI::Win32::VWin32Window::VWin32Window()
{}

VolumeRaytracer::UI::Win32::VWin32Window::~VWin32Window()
{}

void VolumeRaytracer::UI::Win32::VWin32Window::SetTitle(const std::wstring& title)
{
	SetWindowTextW(GetHWND(), title.c_str());
}

void VolumeRaytracer::UI::Win32::VWin32Window::SetSize(const unsigned int& width, const unsigned int& height)
{
	
}

unsigned int VolumeRaytracer::UI::Win32::VWin32Window::GetWidth() const
{
	return Width;
}

unsigned int VolumeRaytracer::UI::Win32::VWin32Window::GetHeight() const
{
	return Height;
}

HWND VolumeRaytracer::UI::Win32::VWin32Window::GetHWND() const
{
	return WindowHandle;
}

HINSTANCE VolumeRaytracer::UI::Win32::VWin32Window::GetHInstance() const
{
	return HInstance;
}

LRESULT CALLBACK VolumeRaytracer::UI::Win32::VWin32Window::MessageHandler(HWND hwnd, UINT msg, WPARAM p1, LPARAM p2)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		Close();
		break;
	}
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		Close();
		break;
	}
	case WM_SYSCHAR:
	{
		break;
	}
	case WM_SIZE:
	{
		//RECT windowRect = {};

		//GetClientRect(hwnd, &windowRect);

		//OnSizeChanged(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}
	break;
	}
	return DefWindowProcW(hwnd, msg, p1, p2);
}

void VolumeRaytracer::UI::Win32::VWin32Window::InitializeWindow()
{
	WNDCLASSEXW wcex;

	ZeroMemory(&wcex, sizeof(wcex));

	HInstance = GetModuleHandleW(NULL);

	Width = 896;
	Height = 504;

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = HInstance;
	wcex.hIcon = LoadIcon(HInstance, MAKEINTRESOURCE(IDI_APPICON));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"VolumeRaytracer";
	wcex.hIconSm = LoadIcon(HInstance, MAKEINTRESOURCE(IDI_APPICON));
	wcex.cbSize = sizeof(WNDCLASSEXW);

	if (!RegisterClassExW(&wcex))
	{
		wchar_t buf[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

		OutputDebugStringW(L"Window registration failed!\n");
		OutputDebugStringW(buf);
	}

	WindowHandle = CreateWindowExW(NULL, L"VolumeRaytracer", L"VolumeRaytracer", WS_OVERLAPPEDWINDOW, 0, 0, Width, Height, NULL, NULL, HInstance, NULL);

	WinMessageDistributor.RegisterWindow(this);

	if (!WindowHandle)
	{
		wchar_t buf[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

		OutputDebugStringW(std::wstring(L"Window creation failed!\n").c_str());
		OutputDebugStringW(buf);
	}

	//VO_LOG(L"Opening new win32 window");

	ShowWindow(WindowHandle, SW_SHOW);
	UpdateWindow(WindowHandle);

	VWindow::InitializeWindow();
}

void VolumeRaytracer::UI::Win32::VWin32Window::CloseWindow()
{
	WinMessageDistributor.UnregisterWindow(this);

	DestroyWindow(WindowHandle);
	WindowHandle = nullptr;

	UnregisterClassW(std::wstring(L"VolumeRaytracer").c_str(), HInstance);
	HInstance = nullptr;

	VWindow::CloseWindow();
}

void VolumeRaytracer::UI::Win32::VWin32Window::Tick(const float& deltaSeconds)
{
	VWindow::Tick(deltaSeconds);

	MSG message;

	ZeroMemory(&message, sizeof(MSG));

	if (PeekMessageW(&message, WindowHandle, 0, 0, PM_REMOVE))
	{
		DispatchMessageW(&message);
	}
}


