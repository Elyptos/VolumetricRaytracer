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

#include "Logger.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <iostream>
#include <assert.h>

void VolumeRaytracer::VLogger::Initialize()
{
	try
	{
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_level(spdlog::level::trace);

		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(RelativeFilePath, true);
		fileSink->set_level(spdlog::level::trace);

		spdlog::sinks_init_list sinkList = {fileSink, consoleSink};

		SpdLogger = std::make_shared<spdlog::logger>("appLog", sinkList.begin(), sinkList.end());
		SpdLogger->flush_on(spdlog::level::info);

		Initialized = true;
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cerr << "Log initialization failed: " << ex.what() << std::endl;
	}
}

void VolumeRaytracer::VLogger::BeginDestroy()
{
	
}

void VolumeRaytracer::VLogger::Log(const std::string& message, ELogType type /*= ELogType::LogDefault*/)
{
	if (IsInitialized())
	{
		switch (type)
		{
		case ELogType::LogDefault:
			SpdLogger->info(message);
			break;
		case ELogType::LogWarning:
			SpdLogger->warn(message);
			break;
		case ELogType::LogError:
			SpdLogger->error(message);
			break;
		case ELogType::LogFatal:
			SpdLogger->error(message);
			break;
		}

		assert(type != ELogType::LogFatal);
	}
	else
	{
		HandleNotInitializedError();
	}
}

void VolumeRaytracer::VLogger::LogWithDefaultLogger(const std::string& message, ELogType type /*= ELogType::LogDefault*/)
{
	if (IsDefaultLoggerSet())
	{
		DefaultLogger->Log(message, type);
	}
	else
	{
		std::cerr << "Default logger has not been set!" << std::endl;
	}
}

void VolumeRaytracer::VLogger::SetAsDefaultLogger(const VObjectPtr<VLogger>& logger)
{
	DefaultLogger = logger;
}

bool VolumeRaytracer::VLogger::IsDefaultLoggerSet()
{
	return DefaultLogger != nullptr;
}

void VolumeRaytracer::VLogger::HandleNotInitializedError()
{
	std::cerr << "Logger not initialized yet!" << std::endl;
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::VLogger> VolumeRaytracer::VLogger::DefaultLogger;
