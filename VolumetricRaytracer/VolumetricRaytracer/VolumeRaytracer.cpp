
#include <memory>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Window.h"
#include "WindowFactory.h"
#include "Object.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	VolumeRaytracer::VObjectPtr<VolumeRaytracer::UI::VWindow> window = VolumeRaytracer::UI::VWindowFactory::NewWindow();

	return 0;
}

#endif