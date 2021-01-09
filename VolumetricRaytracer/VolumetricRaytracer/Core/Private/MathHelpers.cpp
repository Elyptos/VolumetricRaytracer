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

#include "MathHelpers.h"

VolumeRaytracer::VIntVector VolumeRaytracer::VMathHelpers::Index1DTo3D(const size_t& index, const size_t& yCount, const size_t& zCount)
{
	VIntVector res;

	Index1DTo3D(index, yCount, zCount, res.X, res.Y, res.Z);

	return res;
}

void VolumeRaytracer::VMathHelpers::Index1DTo3D(const size_t& index, const size_t& yCount, const size_t& zCount, int& outX, int& outY, int& outZ)
{
	outX = (index / (yCount * zCount));
	outZ = ((index - outX * yCount * zCount) / yCount);
	outY = (index - outX * yCount * zCount - outZ * yCount);
}

size_t VolumeRaytracer::VMathHelpers::Index3DTo1D(const VIntVector& index, const size_t& yCount, const size_t& zCount)
{
	return Index3DTo1D(index.X, index.Y, index.Z, yCount, zCount);
}

size_t VolumeRaytracer::VMathHelpers::Index3DTo1D(const int& x, const int& y, const int& z, const size_t& yCount, const size_t& zCount)
{
	return x * yCount * zCount + z * yCount + y;
}
