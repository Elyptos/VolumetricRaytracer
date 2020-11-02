#include "WindowFactory.h"

#ifdef _WIN32
#include "Win32Window.h"
#endif

#include "Window.h"

VolumeRaytracer::VObjectPtr<VolumeRaytracer::UI::VWindow> VolumeRaytracer::UI::VWindowFactory::NewWindow()
{
#ifdef _WIN32
	VObjectPtr<VWindow> res = VolumeRaytracer::VObjectPtr<VWindow>(VObject::CreateObject<Win32::VWin32Window>());
	return res;
#else
	//Not implemented yet
#endif
}
