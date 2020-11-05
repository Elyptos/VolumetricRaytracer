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

#include "../Public/RendererEngineInstance.h"
#include "WindowFactory.h"
#include "Window.h"
#include "Engine.h"

void VolumeRaytracer::App::RendererEngineInstance::OnEngineInitialized(Engine::VEngine* owningEngine)
{
	Engine = owningEngine;

	CreateRendererWindow();
	Window->Show();
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineShutdown()
{
	Engine = nullptr;
	
	if (Window.IsValid())
	{
		Window->Close();

		OnWindowClosedHandle.disconnect();
		Window = nullptr;
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineUpdate(const float& deltaTime)
{
}

void VolumeRaytracer::App::RendererEngineInstance::CreateRendererWindow()
{
	Window = VolumeRaytracer::UI::VWindowFactory::NewWindow();
	OnWindowClosedHandle = Window->OnWindowClosed_Bind(boost::bind(&RendererEngineInstance::OnWindowClosed, this));
}

void VolumeRaytracer::App::RendererEngineInstance::OnWindowClosed()
{
	if (Engine != nullptr)
	{
		Engine->Shutdown();
	}
}

