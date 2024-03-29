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
#include "Vector.h"
#include <Eigen/Geometry>

namespace VolumeRaytracer
{
	struct VQuat
	{
	public:
		VQuat();
		VQuat(const float& x, const float& y, const float& z, const float& w);
		VQuat(const Eigen::Quaternionf eigenQuat);

		static VQuat FromAxisAngle(const VVector& axis, const float& angle);
		static VQuat FromTwoVectors(const VVector& vec1, const VVector& vec2);
		static VQuat FromForwardVector(const VVector& vec);
		static VQuat FromRightVector(const VVector& vec);
		static VQuat FromUpVector(const VVector& vec);
		static VQuat FromEulerAngles(const float& roll, const float& yaw, const float& pitch);
		static VQuat FromEulerAnglesDegrees(const float& roll, const float& yaw, const float& pitch);

		VQuat Inverse() const;

		VVector GetUpVector();
		VVector GetForwardVector();
		VVector GetRightVector();

		VVector operator*(const VVector& vec) const;
		VQuat operator*(const VQuat& other) const;

		float GetX() const;
		float GetY() const;
		float GetZ() const;
		float GetW() const;

	public:
		static const VQuat IDENTITY;

	private:
		Eigen::Quaternionf EigenQuat;
	};
}