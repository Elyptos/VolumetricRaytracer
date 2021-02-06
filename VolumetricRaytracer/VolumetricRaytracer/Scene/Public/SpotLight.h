/*
	Copyright (c) 2021 Thomas Sch�ngrundner

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

#include "Light.h"

namespace VolumeRaytracer
{
	namespace Scene
	{
		class VSpotLight : public VLight
		{
		public:
			float AttenuationLinear = 0.5f;
			float AttenuationExp = 0.005f;
			float FalloffAngle = 20.f;
			float Angle = 45.f;

		protected:
			std::shared_ptr<VSerializationArchive> Serialize() const override;
			void Deserialize(std::shared_ptr<VSerializationArchive> archive) override;
		};
	}
}