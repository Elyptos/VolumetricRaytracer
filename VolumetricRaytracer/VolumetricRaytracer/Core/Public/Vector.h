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
#include <Eigen/Geometry>

namespace VolumeRaytracer
{
	struct VVector
	{
	public:
		VVector();
		VVector(const float& x, const float& y, const float& z);
		VVector(const Eigen::Vector3f& eigenVector);
		VVector(const VVector& other);

		float Length() const;
		float LengthSquared() const;
		float Dot(const VVector& other) const;
		VVector Cross(const VVector& other) const;
		
		static VVector Cross(const VVector& a, const VVector& b);

		VVector Abs() const;
		static VVector Max(const VVector& vec, const float& scalar);
		static VVector Min(const VVector& vec, const float& scalar);

		void Normalize();
		VVector GetNormalized() const;

		VVector operator+(const VVector& other) const;
		VVector operator-(const VVector& other) const;
		VVector operator-() const;
		VVector operator*(const VVector& other) const;
		VVector operator*(const float& scalar) const;
		VVector operator/(const VVector& other) const;
		VVector operator/(const float& scalar) const;

		operator Eigen::Vector3f() const {return Eigen::Vector3f(X, Y, Z);}

		static VVector Lerp(const VVector& a, const VVector& b, const float& t);

		static VVector PlaneProjection(const VVector& vec, const VVector& planeNormal);
		static VVector VectorProjection(const VVector& vec, const VVector& target);

	public:
		static const VVector ZERO;
		static const VVector ONE;
		static const VVector UP;
		static const VVector RIGHT;
		static const VVector FORWARD;

		float X;
		float Y;
		float Z;
	};
}