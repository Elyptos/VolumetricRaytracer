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

#include "Object.h"
#include "RaytracingHlsl.h"

namespace VolumeRaytracer
{
	namespace Scene
	{
		class VPointLight;
		class VSpotLight;
	}

	namespace Renderer
	{
		namespace DX
		{
			class VDXLightFactory
			{
			public:
				static VPointLightBuffer GetLightBuffer(Scene::VPointLight* light);
				static VSpotLightBuffer GetLightBuffer(Scene::VSpotLight* light);
			};
		}
	}
}