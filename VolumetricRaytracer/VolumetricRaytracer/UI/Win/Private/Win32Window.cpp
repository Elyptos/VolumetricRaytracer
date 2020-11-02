#include "Win32Window.h"
#include "Win32Resources.h"

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

				}

				void UnregisterWindow(VWin32Window* window)
				{

				}
			};

			VWin32MessageDistributor WinMessageDistributor;
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	/*if (VolumeRaytracer::UI::Win32::WinMessageDistributor.RegisteredWindows.find(hWnd) != V_UI::WINMessageDistributor.RegisteredWindows.end())
	{
		return V_UI::WINMessageDistributor.RegisteredWindows[hWnd]->MessageHandler(hWnd, message, wParam, lParam);
	}*/

	return DefWindowProcW(hWnd, message, wParam, lParam);
}

VolumeRaytracer::UI::Win32::VWin32Window::VWin32Window()
{}

VolumeRaytracer::UI::Win32::VWin32Window::~VWin32Window()
{}

void VolumeRaytracer::UI::Win32::VWin32Window::SetTitle(const std::wstring& title)
{
	
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

void VolumeRaytracer::UI::Win32::VWin32Window::InitializeWindow()
{
	WNDCLASSEXW wcex;

	ZeroMemory(&wcex, sizeof(wcex));

	HInstance = GetModuleHandleW(NULL);

	Width = 800;
	Height = 800;

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
}

void VolumeRaytracer::UI::Win32::VWin32Window::CloseWindow()
{
	WinMessageDistributor.UnregisterWindow(this);

	DestroyWindow(WindowHandle);
	WindowHandle = nullptr;

	UnregisterClassW(std::wstring(L"VolumeRaytracer").c_str(), HInstance);
	HInstance = nullptr;
}

void VolumeRaytracer::UI::Win32::VWin32Window::Tick(const float& deltaSeconds)
{
}


