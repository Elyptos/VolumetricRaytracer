#include "Window.h"

VolumeRaytracer::UI::VWindow::VWindow()
{
}

VolumeRaytracer::UI::VWindow::~VWindow()
{
}

void VolumeRaytracer::UI::VWindow::Initialize()
{
	InitializeWindow();
}

void VolumeRaytracer::UI::VWindow::BeginDestroy()
{
	CloseWindow();
}

bool VolumeRaytracer::UI::VWindow::CanEverTick() const
{
	return true;
}

bool VolumeRaytracer::UI::VWindow::ShouldTick() const
{
	return true;
}

