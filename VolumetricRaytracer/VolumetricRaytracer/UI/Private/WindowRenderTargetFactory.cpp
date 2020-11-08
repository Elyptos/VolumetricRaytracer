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
#include "DXRenderTarget.h"
#include "Win32Window.h"
#include "Logger.h"

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Renderer::VRenderTarget> VolumeRaytracer::UI::VWindowRenderTargetFactory::CreateRenderTarget(std::weak_ptr<Renderer::VRenderer> renderer, std::weak_ptr<VWindow> window)
{
	std::shared_ptr<Win32::VWin32Window> win32Window = std::dynamic_pointer_cast<Win32::VWin32Window>(window.lock());

	if (win32Window == nullptr)
	{
		V_LOG_ERROR("Unable to create render target for window! DirectX render targets are currently only supported by Win32 windows.");
		return nullptr;
	}

	std::shared_ptr<Renderer::DX::VDXRenderer> dxRenderer = std::dynamic_pointer_cast<Renderer::DX::VDXRenderer>(renderer.lock());

	if (dxRenderer == nullptr)
	{
		V_LOG_ERROR("Unable to create render target for window! Provided renderer does not support DirectX render targets.");
		return nullptr;
	}

	VObjectPtr<VolumeRaytracer::Renderer::VRenderTarget> renderTarget = nullptr;
	renderTarget = dxRenderer->CreateViewportRenderTarget(win32Window->GetHWND(), win32Window->GetWidth(), win32Window->GetHeight());

	if (renderTarget)
	{
		V_LOG_ERROR("Unable to create render target for window!");
		return nullptr;
	}

	return renderTarget;
}

#endif
