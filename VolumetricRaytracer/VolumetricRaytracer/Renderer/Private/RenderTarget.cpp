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

#include "RenderTarget.h"

VolumeRaytracer::Renderer::VRenderTarget::~VRenderTarget()
{
}

void VolumeRaytracer::Renderer::VRenderTarget::Release()
{
	OnRenderTargetReleased(this);
}

unsigned int VolumeRaytracer::Renderer::VRenderTarget::GetBufferCount() const
{
	return BufferCount;
}

unsigned int VolumeRaytracer::Renderer::VRenderTarget::GetBufferIndex() const
{
	return BufferIndex;
}

void VolumeRaytracer::Renderer::VRenderTarget::SetBufferIndex(unsigned int& bufferIndex)
{
	if (bufferIndex >= 0 && bufferIndex < GetBufferCount())
	{
		BufferIndex = bufferIndex;
	}
}

boost::signals2::connection VolumeRaytracer::Renderer::VRenderTarget::OnRenderTargetReleased_Bind(const GenericRenderTargetDelegate::slot_type& del)
{
	return OnRenderTargetReleased.connect(del);
}

void VolumeRaytracer::Renderer::VRenderTarget::Initialize()
{
	
}

void VolumeRaytracer::Renderer::VRenderTarget::BeginDestroy()
{
	Release();
}
