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
#include "MathHelpers.h"

VolumeRaytracer::VVector VolumeRaytracer::VVector::Lerp(const VVector& a, const VVector& b, const float& t)
{
	return VVector(
		a.X + (b.X - a.X) * t,
		a.Y + (b.Y - a.Y) * t,
		a.Z + (b.Z - a.Z) * t
	);
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::PlaneProjection(const VVector& vec, const VVector& planeNormal)
{
	return vec - VVector::VectorProjection(vec, planeNormal);
}

VolumeRaytracer::VVector VolumeRaytracer::VVector::VectorProjection(const VVector& vec, const VVector& target)
{
	return target * vec.Dot(target);
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

VolumeRaytracer::VVector VolumeRaytracer::VVector::Cross(const VVector& a, const VVector& b)
{
	return a.Cross(b);
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

VolumeRaytracer::VVector VolumeRaytracer::VVector::operator-() const
{
	VVector res;

	res.X = -X;
	res.Y = -Y;
	res.Z = -Z;

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

VolumeRaytracer::VVector2D::VVector2D()
	: VVector2D(0.f, 0.f)
{}

float VolumeRaytracer::VVector2D::Length() const
{
	return sqrt(LengthSquared());
}

float VolumeRaytracer::VVector2D::LengthSquared() const
{
	return X*X + Y*Y;
}

float VolumeRaytracer::VVector2D::Dot(const VVector2D& other) const
{
	return X * other.X + Y * other.Y;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::Abs() const
{
	return VVector2D(abs(X), abs(Y));
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::Max(const VVector2D& vec, const float& scalar)
{
	return VVector2D(VMathHelpers::Max(vec.X, scalar), VMathHelpers::Max(vec.Y, scalar));
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::Min(const VVector2D& vec, const float& scalar)
{
	return VVector2D(VMathHelpers::Min(vec.X, scalar), VMathHelpers::Min(vec.Y, scalar));
}

void VolumeRaytracer::VVector2D::Normalize()
{
	float length = Length();

	if (length < 0.000001f)
	{
		X = 1.f;
		Y = 0.f;
	}
	else
	{
		X /= length;
		Y /= length;
	}
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::GetNormalized() const
{
	VVector2D res = *this;
	res.Normalize();

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator+(const VVector2D& other) const
{	
	VVector2D res;

	res.X = X + other.X;
	res.Y = Y + other.Y;

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator-(const VVector2D& other) const
{
	VVector2D res;

	res.X = X - other.X;
	res.Y = Y - other.Y;

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator*(const VVector2D& other) const
{
	VVector2D res;

	res.X = X * other.X;
	res.Y = Y * other.Y;

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator/(const VVector2D& other) const
{
	VVector2D res;

	res.X = X / other.X;
	res.Y = Y / other.Y;

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::Lerp(const VVector2D& a, const VVector2D& b, const float& t)
{
	return VVector2D(
		VMathHelpers::Lerp(a.X, b.X, t),
		VMathHelpers::Lerp(a.Y, b.Y, t)
	);
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::VectorProjection(const VVector2D& vec, const VVector2D& target)
{
	return target * vec.Dot(target);
}

const VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::ZERO = VVector2D(0.f, 0.f);

const VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::ONE = VVector2D(1.f, 1.f);

const VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::UP = VVector2D(0.f, 1.f);

const VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::RIGHT = VVector2D(1.f, 0.f);

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator/(const float& scalar) const
{
	VVector2D res;

	res.X = X / scalar;
	res.Y = Y / scalar;

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator*(const float& scalar) const
{
	VVector2D res;

	res.X = X * scalar;
	res.Y = Y * scalar;

	return res;
}

VolumeRaytracer::VVector2D VolumeRaytracer::VVector2D::operator-() const
{
	return VVector2D(-X, -Y);
}

VolumeRaytracer::VVector2D::VVector2D(const VVector& other)
	:VVector2D(other.X, other.Y)
{}

VolumeRaytracer::VVector2D::VVector2D(const VVector2D& other)
	:VVector2D(other.X, other.Y)
{}

VolumeRaytracer::VVector2D::VVector2D(const Eigen::Vector2f& eigenVector)
	:VVector2D(eigenVector.x(), eigenVector.y())
{}

VolumeRaytracer::VVector2D::VVector2D(const float& x, const float& y)
	:X(x),
	Y(y)
{}

VolumeRaytracer::VIntVector::VIntVector()
	:VIntVector(0, 0, 0)
{}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::Abs() const
{
	return VIntVector(abs(X), abs(Y), abs(Z));
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::Max(const VIntVector& vec, const int& scalar)
{
	return VIntVector(
		VMathHelpers::Max(vec.X, scalar),
		VMathHelpers::Max(vec.Y, scalar),
		VMathHelpers::Max(vec.Z, scalar)
	);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::Max(const VIntVector& vec1, const VIntVector& vec2)
{
	return VIntVector(
		VMathHelpers::Max(vec1.X, vec2.X),
		VMathHelpers::Max(vec1.Y, vec2.Y),
		VMathHelpers::Max(vec1.Z, vec2.Z)
	);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::Min(const VIntVector& vec, const int& scalar)
{
	return VIntVector(
		VMathHelpers::Min(vec.X, scalar),
		VMathHelpers::Min(vec.Y, scalar),
		VMathHelpers::Min(vec.Z, scalar)
	);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::Min(const VIntVector& vec1, const VIntVector& vec2)
{
	return VIntVector(
		VMathHelpers::Min(vec1.X, vec2.X),
		VMathHelpers::Min(vec1.Y, vec2.Y),
		VMathHelpers::Min(vec1.Z, vec2.Z)
	);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator+(const VIntVector& other) const
{
	return VIntVector(X + other.X, Y + other.Y, Z + other.Z);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator-(const VIntVector& other) const
{
	return VIntVector(X - other.X, Y - other.Y, Z - other.Z);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator*(const VIntVector& other) const
{
	return VIntVector(X * other.X, Y * other.Y, Z * other.Z);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator/(const VIntVector& other) const
{
	return VIntVector(X / other.X, Y / other.Y, Z / other.Z);
}

bool VolumeRaytracer::VIntVector::operator>(const VIntVector& other) const
{
	return X > other.X && Y > other.Y && Z > other.Z;
}

bool VolumeRaytracer::VIntVector::operator<(const VIntVector& other) const
{
	return X < other.X && Y < other.Y && Z < other.Z;
}

bool VolumeRaytracer::VIntVector::operator<=(const VIntVector& other) const
{
	return X <= other.X&& Y <= other.Y && Z <= other.Z;
}

bool VolumeRaytracer::VIntVector::operator>=(const VIntVector& other) const
{
	return X >= other.X && Y >= other.Y && Z >= other.Z;
}

bool VolumeRaytracer::VIntVector::operator==(const VIntVector& other) const
{
	return X == other.X && Y == other.Y && Z == other.Z;
}

bool VolumeRaytracer::VIntVector::operator!=(const VIntVector& other) const
{
	return X != other.X || Y != other.Y || Z != other.Z;
}

const VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::ZERO = VIntVector(0,0,0);

const VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::ONE = VIntVector(1,1,1);

const VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::UP = VIntVector(0,0,1);

const VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::RIGHT = VIntVector(0,1,0);

const VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::FORWARD = VIntVector(1,0,0);

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator/(const int& scalar) const
{
	return VIntVector(X / scalar, Y / scalar, Z / scalar);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator*(const int& scalar) const
{
	return VIntVector(X * scalar, Y * scalar, Z * scalar);
}

VolumeRaytracer::VIntVector VolumeRaytracer::VIntVector::operator-() const
{
	return VIntVector(-X, -Y, -Z);
}

VolumeRaytracer::VIntVector::VIntVector(const VIntVector& other)
	:VIntVector(other.X, other.Y, other.Z)
{}

VolumeRaytracer::VIntVector::VIntVector(const int& x, const int& y, const int& z)
	:X(x),
	Y(y),
	Z(z)
{}
