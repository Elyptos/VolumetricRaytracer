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

#include "Quat.h"
#include "MathHelpers.h"

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

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromTwoVectors(const VVector& vec1, const VVector& vec2)
{
	Eigen::Vector3f from = vec1;
	Eigen::Vector3f to = vec2;

	return Eigen::Quaternionf::FromTwoVectors(from, to);
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromForwardVector(const VVector& vec)
{
	return FromTwoVectors(VVector::FORWARD, vec);
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromRightVector(const VVector& vec)
{
	return FromTwoVectors(VVector::RIGHT, vec);
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromUpVector(const VVector& vec)
{
	return FromTwoVectors(VVector::UP, vec);
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromEulerAngles(const float& roll, const float& yaw, const float& pitch)
{
	return FromAxisAngle(VVector::RIGHT, pitch) * FromAxisAngle(VVector::UP, yaw) * FromAxisAngle(VVector::FORWARD, roll);
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::FromEulerAnglesDegrees(const float& roll, const float& yaw, const float& pitch)
{
	return FromEulerAngles(VMathHelpers::ToRadians(roll), VMathHelpers::ToRadians(yaw), VMathHelpers::ToRadians(pitch));
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::Inverse() const
{
	return EigenQuat.inverse();
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

float VolumeRaytracer::VQuat::GetX() const
{
	return EigenQuat.x();
}

float VolumeRaytracer::VQuat::GetY() const
{
	return EigenQuat.y();
}

float VolumeRaytracer::VQuat::GetZ() const
{
	return EigenQuat.z();
}

float VolumeRaytracer::VQuat::GetW() const
{
	return EigenQuat.w();
}

VolumeRaytracer::VQuat VolumeRaytracer::VQuat::operator*(const VQuat& other) const
{
	return EigenQuat * other.EigenQuat;
}

VolumeRaytracer::VQuat::VQuat(const float& x, const float& y, const float& z, const float& w)
	:EigenQuat(w, x, y, z)
{
}

const VolumeRaytracer::VQuat VolumeRaytracer::VQuat::IDENTITY = VQuat();
