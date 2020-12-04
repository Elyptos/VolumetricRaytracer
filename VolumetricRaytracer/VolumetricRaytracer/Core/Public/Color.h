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

#include "Vector.h"

namespace VolumeRaytracer
{
	struct VColor
	{
	public:
		VColor();
		VColor(const float& r, const float& g, const float& b, const float& a);
		VColor(const VVector& vector, const float& a);
		VColor(const VVector& vector);
		VColor(const VColor& other);

		VColor operator+(const VColor& other) const;
		VColor operator-(const VColor& other) const;
		VColor operator*(const VColor& other) const;
		VColor operator*(const float& scalar) const;
		VColor operator/(const VColor& other) const;
		VColor operator/(const float& scalar) const;

		explicit operator VVector() const { return VVector(R, G, B); }

	public:
		static const VColor BLACK;
		static const VColor WHITE;
		static const VColor RED;
		static const VColor GREEN;
		static const VColor BLUE;

		float R;
		float G;
		float B;
		float A;
	};
}