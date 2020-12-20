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

#include "RDXLevelObject.h"
#include "LevelObject.h"
#include <DirectXMath.h>

VolumeRaytracer::Renderer::DX::VDXLevelObject::VDXLevelObject(const VDXLevelObjectDesc& desc)
	:Desc(desc)
{}

void VolumeRaytracer::Renderer::DX::VDXLevelObject::ChangeGeometry(const size_t& instanceID, const D3D12_GPU_VIRTUAL_ADDRESS& blasHandle)
{
	BLASHandle = blasHandle;
	InstanceID = instanceID;
}

void VolumeRaytracer::Renderer::DX::VDXLevelObject::Update()
{
	Scene::VLevelObject* levelObjectPtr = Desc.LevelObject;

	InstanceDesc.InstanceMask = 1;
	InstanceDesc.InstanceID = InstanceID;
	InstanceDesc.AccelerationStructure = BLASHandle;
	InstanceDesc.InstanceContributionToHitGroupIndex = 0;

	VVector scaleVec = levelObjectPtr->Scale;
	VVector positionVec = levelObjectPtr->Position;
	VQuat rotationQuat = levelObjectPtr->Rotation;

	DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(scaleVec.X, scaleVec.Y, scaleVec.Z);
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(rotationQuat.GetX(), rotationQuat.GetY(), rotationQuat.GetZ(), rotationQuat.GetW()));
	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorSet(positionVec.X, positionVec.Y, positionVec.Z, 0.f));
	DirectX::XMMATRIX transform = rotation * scale * translation;

	DirectX::XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(InstanceDesc.Transform), transform);
}

D3D12_RAYTRACING_INSTANCE_DESC VolumeRaytracer::Renderer::DX::VDXLevelObject::GetInstanceDesc() const
{
	return InstanceDesc;
}

VolumeRaytracer::Renderer::DX::VDXLevelObjectDesc VolumeRaytracer::Renderer::DX::VDXLevelObject::GetObjectDesc() const
{
	return Desc;
}
