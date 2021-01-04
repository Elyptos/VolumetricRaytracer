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

#include "AABB.h"
#include <cmath>

VolumeRaytracer::VAABB::VAABB()
	:VAABB(VVector::ZERO, VVector::ONE * 0.5f)
{}


VolumeRaytracer::VAABB::VAABB(const VAABB& other)
{
	Extends = other.Extends;
	Position = other.Position;
}

VolumeRaytracer::VAABB::VAABB(const VVector& position, const VVector& extends)
{
	Position = position;
	Extends = extends;
}

void VolumeRaytracer::VAABB::SetCenterPosition(const VVector& position)
{
	Position = position;
}

void VolumeRaytracer::VAABB::SetExtends(const VVector& extends)
{
	Extends = VVector(abs(extends.X), abs(extends.Y), abs(extends.Z));
}

VolumeRaytracer::VVector VolumeRaytracer::VAABB::GetMin() const
{
	return Position - Extends;
}

VolumeRaytracer::VVector VolumeRaytracer::VAABB::GetMax() const
{
	return Position + Extends;
}

VolumeRaytracer::VVector VolumeRaytracer::VAABB::GetExtends() const
{
	return Extends;
}

VolumeRaytracer::VVector VolumeRaytracer::VAABB::GetCenterPosition() const
{
	return Position;
}
