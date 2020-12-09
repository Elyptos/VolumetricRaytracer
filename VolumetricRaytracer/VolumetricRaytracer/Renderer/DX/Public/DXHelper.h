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

#include <string>
#include <wrl/client.h>

#include <d3d12.h>

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			template <typename T>
			using CPtr = Microsoft::WRL::ComPtr<T>;

			void SetDXDebugName(ID3D12DeviceChild* elem, const std::string& name);

			template <typename T>
			void SetDXDebugName(CPtr<T> elem, const std::string& name)
			{
				CPtr<ID3D12DeviceChild> child = nullptr;

				if (SUCCEEDED(elem.As(&child)))
				{
					SetDXDebugName(child.Get(), name);
				}
			}

			struct VD3DBuffer
			{
			public:
				CPtr<ID3D12Resource> Resource = nullptr;
				D3D12_CPU_DESCRIPTOR_HANDLE CPUDescHandle;
				D3D12_GPU_DESCRIPTOR_HANDLE GPUDescHandle;

				void Release();
			};

			struct VDXAccelerationStructureBuffers
			{
			public:
				CPtr<ID3D12Resource> Scratch;
				CPtr<ID3D12Resource> AccelerationStructure;
				CPtr<ID3D12Resource> InstanceDesc;
				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc;
				UINT64 ResultDataMaxSizeInBytes;
			};

			class VDXHelper
			{
			public:
				static UINT Align(UINT location, UINT align)
				{
					return (location + (align - 1)) & ~(align - 1);
				}
			};
		}
	}
}