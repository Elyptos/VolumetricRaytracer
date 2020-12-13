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

#include "Vector.h"
#include <cmath>

VolumeRaytracer::VVector VolumeRaytracer::VVector::Lerp(const VVector& a, const VVector& b, const float& t)
{
	return VVector(
		a.X + (b.X - a.X) * t,
		a.Y + (b.Y - a.Y) * t,
		a.Z + (b.Z - a.Z) * t
	);
}

const VolumeRaytracer::VVector VolumeRaytracer::VVector::ZERO = VolumeRaytracer::VVector(0.f, 0.f, 0.f);

const VolumeRaytracer::VVector VolumeRaytracer::VVector::ONE = VolumeRaytracer::VVector(1.f, 1.f, 1.f);

const VolumeRaytracer::VVector VolumeRaytracer::VVector::UP = VolumeRaytracer::VVector(0.f, 0.f, 1.f);

const VolumeRaytracer::VVector VolumeRaytracer::VVector::RIGHT = VolumeRaytracer::VVector(0.f, 1.f, 0.f);

const VolumeRaytracer::VVector VolumeRaytracer::VVector::FORWARD = VolumeRaytracer::VVector(1.f, 0.f, 0.f);




float VolumeRaytracer::VVector::Length() const
{
	return sqrt(LengthSquared());
}

float VolumeRaytracer::VVector::LengthSquared() const
{
	return X*X + Y*Y + Z*Z;
}

float VolumeRaytracer::VVector::Dot(const VVector& other) const
{
	return X*other.X + Y*other.Y + Z*other.Z;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::Cross(const VVector& other) const
{
	return VVector(Y*other.Z - Z*other.Y, Z*other.X - X*other.Z, X*other.Y - Y*other.X);
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::Abs() const
{
	return VVector(abs(X), abs(Y), abs(Z));
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::Max(const VVector& vec, const float& scalar)
{
	float x = vec.X >= scalar ? vec.X : scalar;
	float y = vec.Y >= scalar ? vec.Y : scalar;
	float z = vec.Z >= scalar ? vec.Z : scalar;

	return VVector(x, y, z);
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::Min(const VVector& vec, const float& scalar)
{
	float x = vec.X <= scalar ? vec.X : scalar;
	float y = vec.Y <= scalar ? vec.Y : scalar;
	float z = vec.Z <= scalar ? vec.Z : scalar;

	return VVector(x, y, z);
}

void VolumeRaytracer::VVector::Normalize()
{
	float length = Length();

	if (length < 0.000001f)
	{
		X = 1.f;
		Y = 0.f;
		Z = 0.f;
	}
	else
	{
		X /= length;
		Y /= length;
		Z /= length;
	}
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::GetNormalized() const
{
	VVector res = *this;
	res.Normalize();

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator+(const VVector& other) const
{
	VVector res;

	res.X = X + other.X;
	res.Y = Y + other.Y;
	res.Z = Z + other.Z;

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator-(const VVector& other) const
{
	VVector res;

	res.X = X - other.X;
	res.Y = Y - other.Y;
	res.Z = Z - other.Z;

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator*(const VVector& other) const
{
	VVector res;

	res.X =  X * other.X;
	res.Y =  Y * other.Y;
	res.Z =  Z * other.Z;

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator*(const float& scalar) const
{
	VVector res;

	res.X = X * scalar;
	res.Y = Y * scalar;
	res.Z = Z * scalar;

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator/(const VVector& other) const
{
	VVector res;

	res.X = X / other.X;
	res.Y = Y / other.Y;
	res.Z = Z / other.Z;

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator/(const float& scalar) const
{
	VVector res;

	res.X = X / scalar;
	res.Y = Y / scalar;
	res.Z = Z / scalar;

	return res;
}

VolumeRaytracer::VVector::VVector()
	: VVector(0.f, 0.f, 0.f)
{}

VolumeRaytracer::VVector::VVector(const Eigen::Vector3f& eigenVector)
	: VVector(eigenVector.x(), eigenVector.y(), eigenVector.z())
{}

VolumeRaytracer::VVector::VVector(const VVector& other)
{
	X = other.X;
	Y = other.Y;
	Z = other.Z;
}

VolumeRaytracer::VVector::VVector(const float& x, const float& y, const float& z)
	: X(x),
	Y(y),
	Z(z)
{}
