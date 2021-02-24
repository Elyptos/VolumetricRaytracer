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

#include "Engine.h"
#include "Logger.h"
#include "EngineConstants.h"
#include "IEngineInstance.h"
#include "TickManager.h"
#include "Renderer.h"
#include "RendererFactory.h"
#include <memory>
#include <ctime>
#include <string>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <chrono>
#include <thread>
#include <sstream>
#include <algorithm>

#include "MessageBox.h"


VolumeRaytracer::Engine::VEngine::~VEngine()
{
	Shutdown();
}

void VolumeRaytracer::Engine::VEngine::Start()
{
	if (IsEngineActive() == false && !IsPendingStart)
	{
		IsPendingStart = true;

		InitFPSCounter();
		InitializeLogger();

		if (InitializeRenderer())
		{
			InitializeEngineInstance();
			StartEngineLoop();
		}
		else
		{
			UI::VMessageBox::ShowOk(L"Renderer Initialization Error", L"Renderer failed to initialize!\nPlease make sure your system meets the required system specifications:\n\nGPU: NVIDIA RTX 20xx or newer\nOS: Windows 10", UI::EVMessageBoxType::Error);
			Shutdown();
		}
	}
}

void VolumeRaytracer::Engine::VEngine::Shutdown()
{
	if (IsEngineActive() || IsPendingStart)
	{
		StopEngineLoop();
	}
}

float VolumeRaytracer::Engine::VEngine::GetEngineDeltaTime() const
{
	//return std::max(EngineDeltaTime, 0.00001f);
	return EngineDeltaTime;
}

unsigned int VolumeRaytracer::Engine::VEngine::GetFPS() const
{
	return FPS;
}

void VolumeRaytracer::Engine::VEngine::InitializeLogger()
{
	if (VLogger::IsDefaultLoggerSet() == false)
	{
		VObjectPtr<VLogger> logger = VObject::CreateObject<VLogger>(GetLoggerFilePath());

		VLogger::SetAsDefaultLogger(logger);
	}
}

void VolumeRaytracer::Engine::VEngine::InitializeEngineInstance()
{
	if (EngineInstance != nullptr)
	{
		EngineInstance->OnEngineInitialized(this);
	}
	else
	{
		V_LOG_WARNING("No engine instance specified! Please call VEngine::SetEngineInstance before starting the engine.");
	}
}

bool VolumeRaytracer::Engine::VEngine::InitializeRenderer()
{
	Renderer = Renderer::VRendererFactory::NewRenderer();
	return Renderer->Start();
}

void VolumeRaytracer::Engine::VEngine::InitFPSCounter()
{
	TimeSinceFPSUpdate = 0.f;
	FPS = 0;
	CurrentFrameCount = 0;
}

std::string VolumeRaytracer::Engine::VEngine::GetCurrentDateTimeAsString() const
{
	using namespace std;

	auto time = std::time(nullptr);
	tm ltm;

	localtime_s(&ltm, &time);

	stringstream ss;

	ss << std::put_time(&ltm, "%Y.%m.%d-%H.%M.%S");

	return ss.str();
}

std::string VolumeRaytracer::Engine::VEngine::GetLoggerFilePath() const
{
	using namespace std;

	string time = GetCurrentDateTimeAsString();

	ostringstream os;
	os << "RendererLog_" << time << ".txt";

	boost::filesystem::path folderPath = PATH_LOG;
	boost::filesystem::path filePath = folderPath / os.str();

	return filePath.string();
}

void VolumeRaytracer::Engine::VEngine::CallVObjectTicks(const float& deltaTime)
{
	VGlobalTickManager::Instance().CallTickOnAllAllowedObjects(deltaTime);
}

void VolumeRaytracer::Engine::VEngine::CallPostRenderTicks()
{
	VGlobalTickManager::Instance().CallPostRenderOnAllAllowedObjects();
}

void VolumeRaytracer::Engine::VEngine::TickEngineInstance(const float& deltaTime)
{
	if (EngineInstance != nullptr)
	{
		EngineInstance->OnEngineUpdate(deltaTime);
	}
}

void VolumeRaytracer::Engine::VEngine::ExecuteRenderCommand()
{
	Renderer->Render();
}

void VolumeRaytracer::Engine::VEngine::StartEngineLoop()
{
	if (IsPendingStart == true)
	{
		IsRunning = true;

		IsPendingStart = false;

		V_LOG("Starting engine");

		EngineLoop();
	}
}

void VolumeRaytracer::Engine::VEngine::StopEngineLoop()
{
	V_LOG("Stopping engine");

	IsRunning = false;
	IsPendingStart = false;
}

void VolumeRaytracer::Engine::VEngine::StopRenderer()
{
	if (Renderer != nullptr)
	{
		Renderer->Stop();
		Renderer = nullptr;
	}
}

void VolumeRaytracer::Engine::VEngine::EngineLoop()
{
	while (IsEngineActive())
	{
		auto tStampBegin = std::chrono::high_resolution_clock::now();
		{
			float prevDeltaTime = GetEngineDeltaTime();

			TickEngineInstance(prevDeltaTime);
			CallVObjectTicks(prevDeltaTime);

			ExecuteRenderCommand();

			CallPostRenderTicks();
		}

		//std::this_thread::yield();

		auto tStampEnd = std::chrono::high_resolution_clock::now();
		
		long long frameTime = std::chrono::duration_cast<std::chrono::microseconds>(tStampEnd - tStampBegin).count();
		EngineDeltaTime = frameTime * 0.001f * 0.001f;
		
		//std::this_thread::sleep_for(std::chrono::microseconds(MIN_ENGINE_TICK_TIME_MICROSECONDS));

		CountFPS(EngineDeltaTime);
	}

	V_LOG("Engine loop exited");

	ShutdownEngineAfterEngineLoopFinishes();
}

void VolumeRaytracer::Engine::VEngine::ShutdownEngineAfterEngineLoopFinishes()
{
	if (!IsEngineActive())
	{
		V_LOG("Engine loop stopped! Shutting down...");

		StopRenderer();

		if (EngineInstance != nullptr)
		{
			EngineInstance->OnEngineShutdown();
			EngineInstance = nullptr;
		}
	}
}

void VolumeRaytracer::Engine::VEngine::CountFPS(const float& deltaTime)
{
	CurrentFrameCount++;

	TimeSinceFPSUpdate += deltaTime;

	if (TimeSinceFPSUpdate >= 1.f)
	{
		FPS = CurrentFrameCount;
		CurrentFrameCount = 0;
		TimeSinceFPSUpdate = 0.f;
	}
}
