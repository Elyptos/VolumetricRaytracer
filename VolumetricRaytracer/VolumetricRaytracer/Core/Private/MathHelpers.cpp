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

#include "MathHelpers.h"

void VolumeRaytracer::VMathHelpers::Index1DTo3D(const unsigned int& index, const unsigned int& yCount, const unsigned int& zCount, unsigned int& outX, unsigned int& outY, unsigned int& outZ)
{
	outX = (index / (yCount * zCount));
	outY = ((index - outX * yCount * zCount) / zCount);
	outZ = (index - outX * yCount * zCount - outY * zCount);
}

unsigned int VolumeRaytracer::VMathHelpers::Index3DTo1D(const unsigned int& x, const unsigned int& y, const unsigned int& z, const unsigned int& yCount, const unsigned int& zCount)
{
	return x * yCount * zCount + y * zCount + z;
}