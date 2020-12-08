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
#include "../../Renderer/DX/Public/DXRenderer.h"
#include "../../Core/Public/Logger.h"

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


			enum EVWin32KeyCode
			{
				A = 0x41,
				D = 0x44,
				S = 0x53,
				W = 0x57,
			};
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
		return 0;
	}
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		Close();
		return 0;
	}
	break;
	case WM_SIZE:
	{
		RECT windowRect = {};
		GetClientRect(hwnd, &windowRect);

		OnSizeChanged(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
		return 0;
	}
	break;
	case WM_PAINT:
	{
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		LockMouseCursor();

		return 0;
	}
	case WM_KEYDOWN:
	{
		ProcessKeyDown(p1);
		
		return 0;
	}
	case WM_KEYUP:
	{
		ProcessKeyUp(p1);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		if (IsMouseLocked)
		{
			POINT centerOfWindow = GetCenterOfWindow();
			POINT mousePos;

			if (GetCursorPos(&mousePos))
			{
				MouseXDelta = mousePos.x - centerOfWindow.x;
				MouseYDelta = mousePos.y - centerOfWindow.y;
			}

			RecenterMouseInWindow();
		}

		return 0;
	}
	case WM_KILLFOCUS:
	{
		FreeMouseCursor();
		return 0;
	}
	}
	return DefWindowProcW(hwnd, msg, p1, p2);
}

void VolumeRaytracer::UI::Win32::VWin32Window::LockMouseCursor()
{
	RECT windowRect;
	RECT cursorCaptureRect;

	if (GetWindowRect(WindowHandle, &windowRect))
	{
		//float xPos = windowRect.left + (windowRect.right - windowRect.left) * 0.5f;
		//float yPos = windowRect.top + (windowRect.bottom - windowRect.top) * 0.5f;

		//cursorCaptureRect.top = yPos;
		//cursorCaptureRect.bottom = yPos;
		//cursorCaptureRect.left = xPos;
		//cursorCaptureRect.right = xPos;

		ShowCursor(false);
		SetCapture(WindowHandle);
		ClipCursor(&windowRect);
		IsMouseLocked = true;
	}
}

void VolumeRaytracer::UI::Win32::VWin32Window::FreeMouseCursor()
{
	ClipCursor(nullptr);
	IsMouseLocked = false;
	ReleaseCapture();
	ShowCursor(true);
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

	WindowHandle = CreateWindowExW(NULL, L"VolumeRaytracer", L"VolumeRaytracer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, HInstance, NULL);

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

	FreeMouseCursor();

	DestroyWindow(WindowHandle);
	WindowHandle = nullptr;

	UnregisterClassW(std::wstring(L"VolumeRaytracer").c_str(), HInstance);
	HInstance = nullptr;

	VWindow::CloseWindow();
}

void VolumeRaytracer::UI::Win32::VWin32Window::Tick(const float& deltaSeconds)
{
	VWindow::Tick(deltaSeconds);

	ProcessAxisEvents();

	MSG message;

	ZeroMemory(&message, sizeof(MSG));

	if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

bool VolumeRaytracer::UI::Win32::VWin32Window::AttachToRenderer(Renderer::VRenderer* renderer)
{
	Renderer::DX::VDXRenderer* dxRenderer = dynamic_cast<Renderer::DX::VDXRenderer*>(renderer);

	if (dxRenderer == nullptr)
	{
		V_LOG_ERROR("Unable to attach window to renderer! Provided renderer is not a DirectX renderer!");
		return false;
	}

	dxRenderer->SetWindowHandle(GetHWND(), GetWidth(), GetHeight());

	return true;
}

bool VolumeRaytracer::UI::Win32::VWin32Window::DetachFromRenderer(Renderer::VRenderer* renderer)
{
	Renderer::DX::VDXRenderer* dxRendererPtr = dynamic_cast<Renderer::DX::VDXRenderer*>(renderer);

	if (dxRendererPtr == nullptr)
	{
		V_LOG_ERROR("Unable to detach window from renderer! Renderer is not valid!");
		return false;
	}

	dxRendererPtr->ClearWindowHandle();

	return true;
}

void VolumeRaytracer::UI::Win32::VWin32Window::ProcessKeyDown(WPARAM key)
{
	switch (key)
	{
		case VK_ESCAPE:
		{
			CloseWindow();
		}
		break;
		case EVWin32KeyCode::W:
		{
			OnKeyPressed(EVKeyType::W);
		}
		break;
		case EVWin32KeyCode::A:
		{
			OnKeyPressed(EVKeyType::A);
		}
		break;
		case EVWin32KeyCode::S:
		{
			OnKeyPressed(EVKeyType::S);
		}
		break;
		case EVWin32KeyCode::D:
		{
			OnKeyPressed(EVKeyType::D);
		}
		break;
	}
}

void VolumeRaytracer::UI::Win32::VWin32Window::ProcessKeyUp(WPARAM key)
{
	switch (key)
	{
	break;
	case EVWin32KeyCode::W:
	{
		OnKeyReleased(EVKeyType::W);
	}
	break;
	case EVWin32KeyCode::A:
	{
		OnKeyReleased(EVKeyType::A);
	}
	break;
	case EVWin32KeyCode::S:
	{
		OnKeyReleased(EVKeyType::S);
	}
	break;
	case EVWin32KeyCode::D:
	{
		OnKeyReleased(EVKeyType::D);
	}
	break;
	}
}

void VolumeRaytracer::UI::Win32::VWin32Window::ProcessAxisEvents()
{
	OnAxisInput(EVAxisType::MouseX, MouseXDelta * 0.04f);
	OnAxisInput(EVAxisType::MouseY, MouseYDelta * 0.04f);

	MouseXDelta = 0.f;
	MouseYDelta = 0.f;
}

void VolumeRaytracer::UI::Win32::VWin32Window::RecenterMouseInWindow()
{
	POINT center = GetCenterOfWindow();

	SetCursorPos(center.x, center.y);
}

POINT VolumeRaytracer::UI::Win32::VWin32Window::GetCenterOfWindow() const
{
	POINT res;
	RECT windowRect;

	if (GetWindowRect(WindowHandle, &windowRect))
	{
		res.x = windowRect.left + (windowRect.right - windowRect.left) * 0.5f;
		res.y = windowRect.top + (windowRect.bottom - windowRect.top) * 0.5f;
	}

	return res;
}

