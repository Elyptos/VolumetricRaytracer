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

#include "WindowRenderTargetFactory.h"

#ifdef _WIN32
#include "DXRenderer.h"
#include "Win32Window.h"
#include "Logger.h"

bool VolumeRaytracer::UI::VWindowRenderTargetFactory::AttachWindowToRenderer(std::weak_ptr<VWindow> window, std::weak_ptr<Renderer::VRenderer> renderer)
{
	std::shared_ptr<Win32::VWin32Window> win32Window = std::dynamic_pointer_cast<Win32::VWin32Window>(window.lock());

	if (win32Window == nullptr)
	{
		V_LOG_ERROR("Unable to attach window to renderer! DirectX renderer only supports Win32 windows!");
		return false;
	}

	std::shared_ptr<Renderer::DX::VDXRenderer> dxRenderer = std::dynamic_pointer_cast<Renderer::DX::VDXRenderer>(renderer.lock());

	if (dxRenderer == nullptr)
	{
		V_LOG_ERROR("Unable to attach window to renderer! Provided renderer is no DirectX renderer!");
		return false;
	}

	dxRenderer->SetWindowHandle(win32Window->GetHWND(), win32Window->GetWidth(), win32Window->GetHeight());

	return true;
}

bool VolumeRaytracer::UI::VWindowRenderTargetFactory::DetachWindowFromRenderer(std::weak_ptr<VWindow> window, std::weak_ptr<Renderer::VRenderer> renderer)
{
	std::shared_ptr<VWindow> winPtr = window.lock();
	std::shared_ptr<Renderer::DX::VDXRenderer> dxRendererPtr = std::dynamic_pointer_cast<Renderer::DX::VDXRenderer>(renderer.lock());

	if (winPtr == nullptr)
	{
		V_LOG_ERROR("Unable to detach window from renderer! Window is not valid!");
		return false;
	}

	if (dxRendererPtr == nullptr)
	{
		V_LOG_ERROR("Unable to detach window from renderer! Renderer is not valid!");
		return false;
	}

	if (winPtr->GetAttachedRenderer().lock() != dxRendererPtr)
	{
		V_LOG_ERROR("Unable to detach window from renderer! Window is not attached to the provided renderer!");
		return false;
	}

	dxRendererPtr->ClearWindowHandle();

	return true;
}

#endif
