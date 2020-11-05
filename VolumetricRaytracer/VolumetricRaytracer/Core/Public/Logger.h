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

#pragma once
#include "Object.h"
#include <string>
#include <memory>

#define V_LOG(message) \
	VolumeRaytracer::VLogger::LogWithDefaultLogger(message, VolumeRaytracer::ELogType::LogDefault);

#define V_LOG_WARNING(message) \
	VolumeRaytracer::VLogger::LogWithDefaultLogger(message, VolumeRaytracer::ELogType::LogWarning);

#define V_LOG_ERROR(message) \
	VolumeRaytracer::VLogger::LogWithDefaultLogger(message, VolumeRaytracer::ELogType::LogError);

#define V_LOG_FATAL(message) \
	VolumeRaytracer::VLogger::LogWithDefaultLogger(message, VolumeRaytracer::ELogType::LogFatal);

namespace spdlog
{
	class logger;
}

namespace VolumeRaytracer
{
	enum class ELogType
	{
		LogDefault = 0,
		LogWarning = 1,
		LogError = 2,
		LogFatal = 3
	};

	class VLogger : public VObject
	{
	public:
		VLogger(const std::string& relativeFilePath)
		 :RelativeFilePath(relativeFilePath){}

		void Log(const std::string& message, ELogType type = ELogType::LogDefault);

		bool IsInitialized() const { return Initialized; }

		static void LogWithDefaultLogger(const std::string& message, ELogType type = ELogType::LogDefault);
		static void SetAsDefaultLogger(const VObjectPtr<VLogger>& logger);
		static bool IsDefaultLoggerSet();

	protected:
		void Initialize() override;
		void BeginDestroy() override;

	private:
		void HandleNotInitializedError();

	private:
		std::string RelativeFilePath;
		std::shared_ptr<spdlog::logger> SpdLogger = nullptr;

		bool Initialized = false;

		static VObjectPtr<VLogger> DefaultLogger;
	};
}