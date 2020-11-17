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

#include "Quat.h"

VolumeRaytracer::VQuat::VQuat()
	:EigenQuat(Eigen::Quaternionf::Identity())
{	
}

VolumeRaytracer::VQuat::VQuat(const Eigen::Quaternionf eigenQuat)
	:EigenQuat(eigenQuat)
{
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromAxisAngle(const VVector& axis, const float& angle)
{
	Eigen::Vector3f eigenAxis(axis.X, axis.Y, axis.Z);

	return Eigen::Quaternionf(Eigen::AngleAxisf(angle, eigenAxis));
}

VolumeRaytracer::VVector VolumeRaytracer::VQuat::GetUpVector()
{
	VVector vec = VVector::UP;
	return operator*(vec);
}

VolumeRaytracer::VVector VolumeRaytracer::VQuat::GetForwardVector()
{
	VVector vec = VVector::FORWARD;
	return operator*(vec);
}

VolumeRaytracer::VVector VolumeRaytracer::VQuat::GetRightVector()
{
	VVector vec = VVector::RIGHT;
	return operator*(vec);
}

VolumeRaytracer::VVector VolumeRaytracer::VQuat::operator*(const VVector& vec) const
{
	Eigen::Vector3f eigenVec = vec;
	return EigenQuat * eigenVec;
}

VolumeRaytracer::VQuat::VQuat(const float& x, const float& y, const float& z, const float& w)
	:EigenQuat(w, x, y, z)
{
}

const VolumeRaytracer::VQuat VolumeRaytracer::VQuat::IDENTITY = VQuat();
