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
#include "Object.h"
#include "Vector.h"
#include "Quat.h"

namespace VolumeRaytracer
{
	namespace Scene
	{
		class VCamera : public VObject
		{
		protected:
			void Initialize() override;
			void BeginDestroy() override;

		public:
			float FOVAngle = 60.f;
			float NearClipPlane = 0.01f;
			float FarClipPlane = 125.f;
			float AspectRatio = 1.7777f;
			VVector Position = VVector::ZERO;
			VQuat Rotation = VQuat::IDENTITY;
		};
	}
}