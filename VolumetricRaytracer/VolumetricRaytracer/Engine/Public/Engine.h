/*
	Copyright (c) 2020 Thomas Sch�ngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#pragma once;

#include <string>
#include <memory>

namespace VolumeRaytracer
{
	namespace Renderer
	{
		class VRenderer;
	}

	namespace Engine
	{
		class IEngineInstance;

		class VEngine
		{
		public:
			VEngine() = default;
			~VEngine();

			void SetEngineInstance(std::shared_ptr<IEngineInstance> instance) { EngineInstance = instance; }

			void Start();
			void Shutdown();

			float GetEngineDeltaTime() const;

			bool IsEngineActive() const { return IsRunning; }

			std::weak_ptr<Renderer::VRenderer> GetRenderer() const { return Renderer; }

		private:
			void InitializeLogger();
			void InitializeEngineInstance();
			void InitializeRenderer();

			std::string GetCurrentDateTimeAsString() const;
			std::string GetLoggerFilePath() const;

			void CallVObjectTicks(const float& deltaTime);
			void TickEngineInstance(const float& deltaTime);

			void StartEngineLoop();
			void StopEngineLoop();
			void StopRenderer();
			void EngineLoop();

			void ShutdownEngineAfterEngineLoopFinishes();

		private:
			bool IsRunning = false;
			float EngineDeltaTime = 0.f;

			std::shared_ptr<Renderer::VRenderer> Renderer; 
			std::shared_ptr<IEngineInstance> EngineInstance;
		};
	}
}