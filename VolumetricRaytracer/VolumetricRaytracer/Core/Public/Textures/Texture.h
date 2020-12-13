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

namespace VolumeRaytracer
{
	class VTexture : public VObject
	{
	public:
		virtual ~VTexture() = default;
		size_t GetMipCount() const;

		virtual size_t GetPixelCount() = 0;
		virtual void Commit() = 0;

	protected:
		std::wstring AssetPath;
		size_t MipCount;
	};
}