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

#include "DXHelper.h"

#include <d3d12.h>

void VolumeRaytracer::Renderer::DX::SetDXDebugName(ID3D12DeviceChild* elem, const std::string& name)
{
	if (elem != nullptr)
	{
		elem->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
	}
}

VolumeRaytracer::Renderer::DX::VD3DConstantBuffer::~VD3DConstantBuffer()
{
	Release();
}

void VolumeRaytracer::Renderer::DX::VD3DConstantBuffer::Release()
{
	if (Resource != nullptr)
	{
		Resource.Reset();
		Resource = nullptr;
	}

	DataPtr = nullptr;
}
