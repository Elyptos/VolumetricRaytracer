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
#include "Quat.h"

namespace VolumeRaytracer
{
	struct VAABB
	{
	public:
		VAABB();
		VAABB(const VVector& position, const VVector& extends);
		VAABB(const VAABB& other);

		void SetCenterPosition(const VVector& position);
		void SetExtends(const VVector& extends);

		VVector GetMin() const;
		VVector GetMax() const;

		VVector GetExtends() const;
		VVector GetCenterPosition() const;

		static VAABB Combine(const VAABB& a, const VAABB& b);
		static VAABB Transform(const VAABB& bounds, const VVector& position, const VVector& scale, const VQuat& rotation);

	private:
		VVector Position;
		VVector Extends;
	};
}