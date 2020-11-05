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

#include <string>
#include "Object.h"

namespace VolumeRaytracer
{
	namespace UI
	{
		class VWindow : public VObject
		{
		public:
			VWindow();
			virtual ~VWindow();

			virtual void SetTitle(const std::wstring& title) = 0;
			virtual void SetSize(const unsigned int& width, const unsigned int& height) = 0;

			virtual unsigned int GetWidth() const = 0;
			virtual unsigned int GetHeight() const = 0;

			void Tick(const float& deltaSeconds) override;

		protected:
			virtual void InitializeWindow() = 0;
			virtual void CloseWindow() = 0;

			void Initialize() override;
			void BeginDestroy() override;

			bool const CanEverTick() const override;
			bool ShouldTick() const override;
		};
	}
}