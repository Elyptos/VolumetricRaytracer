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
#include "../../Renderer/Public/Renderer.h"

VolumeRaytracer::UI::VWindow::VWindow()
{
}

VolumeRaytracer::UI::VWindow::~VWindow()
{
}

void VolumeRaytracer::UI::VWindow::SetRenderer(std::weak_ptr<Renderer::VRenderer> renderer)
{
	if (!Renderer.expired() && Renderer.lock() != renderer.lock())
	{
		RemoveRenderer();
	}

	Renderer::VRenderer* rendererPtr = renderer.lock().get();

	if (AttachToRenderer(rendererPtr))
	{
		Renderer = renderer;
	}
}

void VolumeRaytracer::UI::VWindow::RemoveRenderer()
{
	if (!Renderer.expired())
	{
		Renderer::VRenderer* rendererPtr = Renderer.lock().get();

		if (DetachFromRenderer(rendererPtr))
		{
			Renderer.reset();
		}
	}
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

	RemoveRenderer();

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

void VolumeRaytracer::UI::VWindow::OnSizeChanged(const unsigned int& width, const unsigned int& height)
{
	if (!Renderer.expired())
	{
		Renderer.lock()->ResizeRenderOutput(width, height);
	}
}

