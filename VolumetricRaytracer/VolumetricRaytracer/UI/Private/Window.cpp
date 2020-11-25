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

#include "Window.h"
#include "Textures/RenderTarget.h"

VolumeRaytracer::UI::VWindow::VWindow()
{
}

VolumeRaytracer::UI::VWindow::~VWindow()
{
}

void VolumeRaytracer::UI::VWindow::SetRenderTarget(const VObjectPtr<Renderer::VRenderTarget>& renderTarget)
{
	RenderTarget = renderTarget;
}

void VolumeRaytracer::UI::VWindow::Show()
{
	if (!IsWindowOpen())
	{
		InitializeWindow();
	}
}

void VolumeRaytracer::UI::VWindow::Close()
{
	if (IsWindowOpen())
	{
		CloseWindow();
	}
}

void VolumeRaytracer::UI::VWindow::Tick(const float& deltaSeconds)
{
	
}

bool VolumeRaytracer::UI::VWindow::IsWindowOpen() const
{
	return WindowOpen;
}

boost::signals2::connection VolumeRaytracer::UI::VWindow::OnWindowOpened_Bind(const WindowDelegate::slot_type& del)
{
	return OnWindowOpened.connect(del);
}

boost::signals2::connection VolumeRaytracer::UI::VWindow::OnWindowClosed_Bind(const WindowDelegate::slot_type& del)
{
	return OnWindowClosed.connect(del);
}

void VolumeRaytracer::UI::VWindow::InitializeWindow()
{
	WindowOpen = true;

	OnWindowOpened();
}

void VolumeRaytracer::UI::VWindow::CloseWindow()
{
	WindowOpen = false;

	OnWindowClosed();
}

void VolumeRaytracer::UI::VWindow::Initialize()
{}

void VolumeRaytracer::UI::VWindow::BeginDestroy()
{
	Close();
}

const bool VolumeRaytracer::UI::VWindow::CanEverTick() const
{
	return true;
}

bool VolumeRaytracer::UI::VWindow::ShouldTick() const
{
	return IsWindowOpen();
}

