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
#include <vector>

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

VolumeRaytracer::VAABB VolumeRaytracer::VAABB::Combine(const VAABB& a, const VAABB& b)
{
	VVector min = VVector::Min(a.GetMin(), b.GetMin());
	VVector max = VVector::Max(a.GetMax(), b.GetMax());

	VVector extends = (max - min) * 0.5f;

	return VAABB(min + extends, extends);
}

VolumeRaytracer::VAABB VolumeRaytracer::VAABB::Transform(const VAABB& bounds, const VVector& position, const VVector& scale, const VQuat& rotation)
{
	VVector min = bounds.GetMin() - bounds.Position;
	VVector max = bounds.GetMax() - bounds.Position;

	min = min * scale;
	max = max * scale;

	VVector size = bounds.Extends * 2.f;

	std::vector<VVector> corners;

	corners.push_back(min);
	corners.push_back(min + VVector(size.X, 0.f, 0.f));
	corners.push_back(min + VVector(0.f, size.Y, 0.f));
	corners.push_back(min + VVector(size.X, size.Y, 0.f));
	corners.push_back(min + VVector(0.f, 0.f, size.Z));
	corners.push_back(min + VVector(size.X, 0.f, size.Z));
	corners.push_back(min + VVector(0.f, size.Y, size.Z));
	corners.push_back(max);

	for (int i = 0; i < 8; i++)
	{
		VVector vec = corners[i];

		corners[i] = rotation * corners[i];
	}

	min = corners[0];
	max = corners[0];

	for (int i = 0; i < 8; i++)
	{
		VVector& vec = corners[i];

		min = VVector::Min(min, vec);
		max = VVector::Max(max, vec);
	}

	VVector extends = (max - min) * 0.5f;

	return VAABB(position, extends);
}
