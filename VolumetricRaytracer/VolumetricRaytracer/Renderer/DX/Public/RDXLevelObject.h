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

#include "DXHelper.h"

namespace VolumeRaytracer
{
	namespace Scene
	{
		class VLevelObject;
	}

	namespace Renderer
	{
		namespace DX
		{
			struct VDXLevelObjectDesc
			{
			public:
				Scene::VLevelObject* LevelObject;
			};

			class VDXLevelObject
			{
			public:
				VDXLevelObject(const VDXLevelObjectDesc& desc);
				~VDXLevelObject() = default;

				void ChangeGeometry(const size_t& instanceID, const D3D12_GPU_VIRTUAL_ADDRESS& blasHandle);
				void Update();
				D3D12_RAYTRACING_INSTANCE_DESC GetInstanceDesc() const;

				VDXLevelObjectDesc GetObjectDesc() const;

			private:
				VDXLevelObjectDesc Desc;
				D3D12_RAYTRACING_INSTANCE_DESC InstanceDesc;
				size_t InstanceID;
				D3D12_GPU_VIRTUAL_ADDRESS BLASHandle;
			};
		}
	}
}