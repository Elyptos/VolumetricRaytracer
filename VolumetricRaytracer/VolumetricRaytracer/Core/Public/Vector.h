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

	struct VVector2D
	{
	public:
		VVector2D();
		VVector2D(const float& x, const float& y);
		VVector2D(const Eigen::Vector2f& eigenVector);
		VVector2D(const VVector2D& other);
		explicit VVector2D(const VVector& other);

		float Length() const;
		float LengthSquared() const;
		float Dot(const VVector2D& other) const;

		VVector2D Abs() const;
		static VVector2D Max(const VVector2D& vec, const float& scalar);
		static VVector2D Min(const VVector2D& vec, const float& scalar);

		void Normalize();
		VVector2D GetNormalized() const;

		VVector2D operator+(const VVector2D& other) const;
		VVector2D operator-(const VVector2D& other) const;
		VVector2D operator-() const;
		VVector2D operator*(const VVector2D& other) const;
		VVector2D operator*(const float& scalar) const;
		VVector2D operator/(const VVector2D& other) const;
		VVector2D operator/(const float& scalar) const;

		operator Eigen::Vector2f() const { return Eigen::Vector2f(X, Y); }
		operator VVector() const { return VVector(X, Y, 0.f); }

		static VVector2D Lerp(const VVector2D& a, const VVector2D& b, const float& t);

		static VVector2D VectorProjection(const VVector2D& vec, const VVector2D& target);

	public:
		static const VVector2D ZERO;
		static const VVector2D ONE;
		static const VVector2D UP;
		static const VVector2D RIGHT;

		float X;
		float Y;
	};

	struct VIntVector
	{
	public:
		VIntVector();
		VIntVector(const int& x, const int& y, const int& z);
		VIntVector(const VIntVector& other);

		VIntVector Abs() const;
		static VIntVector Max(const VIntVector& vec, const int& scalar);
		static VIntVector Max(const VIntVector& vec1, const VIntVector& vec2);
		static VIntVector Min(const VIntVector& vec, const int& scalar);
		static VIntVector Min(const VIntVector& vec1, const VIntVector& vec2);

		VIntVector operator+(const VIntVector& other) const;
		VIntVector operator-(const VIntVector& other) const;
		VIntVector operator-() const;
		VIntVector operator*(const VIntVector& other) const;
		VIntVector operator*(const int& scalar) const;
		VIntVector operator/(const VIntVector& other) const;
		VIntVector operator/(const int& scalar) const;

		bool operator==(const VIntVector& other) const;
		bool operator!=(const VIntVector& other) const;
		bool operator>=(const VIntVector& other) const;
		bool operator<=(const VIntVector& other) const;
		bool operator>(const VIntVector& other) const;
		bool operator<(const VIntVector& other) const;

	public:
		static const VIntVector ZERO;
		static const VIntVector ONE;
		static const VIntVector UP;
		static const VIntVector RIGHT;
		static const VIntVector FORWARD;

		int X;
		int Y;
		int Z;
	};
}