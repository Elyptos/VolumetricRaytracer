/*
	Copyright (c) 2021 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#include "DXLightFactory.h"
#include "PointLight.h"
#include "SpotLight.h"

VolumeRaytracer::VPointLightBuffer VolumeRaytracer::Renderer::DX::VDXLightFactory::GetLightBuffer(Scene::VPointLight* light)
{
	VPointLightBuffer buffer;

	buffer.position = DirectX::XMVectorSet(light->Position.X, light->Position.Y, light->Position.Z, 1.f);
	buffer.falloffDistance = light->FalloffDistance;
	buffer.lightIntensity = light->IlluminationStrength;
	buffer.color = DirectX::XMFLOAT4(light->Color.R, light->Color.G, light->Color.B, 1.f);

	return buffer;
}

VolumeRaytracer::VSpotLightBuffer VolumeRaytracer::Renderer::DX::VDXLightFactory::GetLightBuffer(Scene::VSpotLight* light)
{
	VSpotLightBuffer buffer;

	buffer.position = DirectX::XMVectorSet(light->Position.X, light->Position.Y, light->Position.Z, 1.f);
	buffer.color = DirectX::XMFLOAT4(light->Color.R, light->Color.G, light->Color.B, 1.f);
	buffer.falloffDistance = light->FalloffDistance;

	VVector lightForward = light->Rotation.GetForwardVector();

	buffer.forward = DirectX::XMVectorSet(lightForward.X, lightForward.Y, lightForward.Z, 1.f);;
	buffer.angle = light->Angle;

	return buffer;
}
