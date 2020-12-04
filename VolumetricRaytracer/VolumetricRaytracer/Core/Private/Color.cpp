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

#include "Color.h"

const VolumeRaytracer::VColor VolumeRaytracer::VColor::BLACK = VColor(0.f, 0.f, 0.f, 1.f);

const VolumeRaytracer::VColor VolumeRaytracer::VColor::WHITE = VColor(1.f, 1.f, 1.f, 1.f);

const VolumeRaytracer::VColor VolumeRaytracer::VColor::RED = VColor(1.f, 0.f, 0.f, 1.f);

const VolumeRaytracer::VColor VolumeRaytracer::VColor::GREEN = VColor(0.f, 1.f, 0.f, 1.f);

const VolumeRaytracer::VColor VolumeRaytracer::VColor::BLUE = VColor(0.f, 0.f, 1.f, 1.f);


VolumeRaytracer::VColor::VColor()
	:VColor(0.f, 0.f, 0.f, 0.f)
{}


VolumeRaytracer::VColor::VColor(const VColor& other)
	:R(other.R),
	G(other.G),
	B(other.B),
	A(other.A)
{
}

VolumeRaytracer::VColor::VColor(const VVector& vector)
	:VColor(vector, 1.f)
{}

VolumeRaytracer::VColor::VColor(const VVector& vector, const float& a)
	:R(vector.X),
	G(vector.Y),
	B(vector.Z),
	A(a)
{}

VolumeRaytracer::VColor::VColor(const float& r, const float& g, const float& b, const float& a)
	:R(r),
	G(g),
	B(b),
	A(a)
{}


VolumeRaytracer::VColor VolumeRaytracer::VColor::operator+(const VColor& other) const
{
	VColor res;

	res.R = R + other.R;
	res.G = G + other.G;
	res.B = B + other.B;
	res.A = A + other.A;

	return res;
}

VolumeRaytracer::VColor VolumeRaytracer::VColor::operator-(const VColor& other) const
{
	VColor res;

	res.R = R - other.R;
	res.G = G - other.G;
	res.B = B - other.B;
	res.A = A - other.A;

	return res;
}

VolumeRaytracer::VColor VolumeRaytracer::VColor::operator*(const VColor& other) const
{
	VColor res;

	res.R = R * other.R;
	res.G = G * other.G;
	res.B = B * other.B;
	res.A = A * other.A;

	return res;
}

VolumeRaytracer::VColor VolumeRaytracer::VColor::operator/(const VColor& other) const
{
	VColor res;

	res.R = R / other.R;
	res.G = G / other.G;
	res.B = B / other.B;
	res.A = A / other.A;

	return res;
}

VolumeRaytracer::VColor VolumeRaytracer::VColor::operator/(const float& scalar) const
{
	VColor res;

	res.R = R / scalar;
	res.G = G / scalar;
	res.B = B / scalar;
	res.A = A / scalar;

	return res;
}

VolumeRaytracer::VColor VolumeRaytracer::VColor::operator*(const float& scalar) const
{
	VColor res;

	res.R = R * scalar;
	res.G = G * scalar;
	res.B = B * scalar;
	res.A = A * scalar;

	return res;
}
