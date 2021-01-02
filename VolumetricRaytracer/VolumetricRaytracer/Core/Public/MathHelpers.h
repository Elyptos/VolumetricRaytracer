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
	class VMathHelpers
	{
	public:
		static void Index1DTo3D(const size_t& index, const size_t& yCount, const size_t& zCount, int& outX, int& outY, int& outZ);
		static VIntVector Index1DTo3D(const size_t& index, const size_t& yCount, const size_t& zCount);
		static size_t Index3DTo1D(const int& x, const int& y, const int& z, const size_t& yCount, const size_t& zCount);
		static size_t Index3DTo1D(const VIntVector& index, const size_t& yCount, const size_t& zCount);

		template<typename T>
		static T Clamp(const T& a, const T& min, const T& max)
		{
			T res = a <= min ? min : a;
			return res >= max ? max : res;
		}

		template<typename T>
		static T Min(const T& a, const T& b)
		{
			return a <= b ? a : b;
		}

		template<typename T>
		static T Max(const T& a, const T& b)
		{
			return a >= b ? a : b;
		}

		template<typename T>
		static int Sign(T val)
		{
			return (T(0) < val) - (val < T(0));
		}

		template<typename T>
		static T Lerp(const T& a, const T& b, const float& t)
		{
			return a + (b - a) * t;
		}
	};
}