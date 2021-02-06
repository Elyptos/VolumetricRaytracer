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

#include "Light.h"

std::shared_ptr<VolumeRaytracer::VSerializationArchive> VolumeRaytracer::Scene::VLight::Serialize() const
{
	std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();
	res->BufferSize = 0;
	res->Buffer = nullptr;

	std::shared_ptr<VSerializationArchive> pos = std::make_shared<VSerializationArchive>();
	std::shared_ptr<VSerializationArchive> scale = std::make_shared<VSerializationArchive>();
	std::shared_ptr<VSerializationArchive> rot = std::make_shared<VSerializationArchive>();
	std::shared_ptr<VSerializationArchive> color = VSerializationArchive::From<VColor>(&Color);
	std::shared_ptr<VSerializationArchive> strength = VSerializationArchive::From<float>(&IlluminationStrength);

	pos->BufferSize = sizeof(VVector);
	scale->BufferSize = sizeof(VVector);
	rot->BufferSize = sizeof(VQuat);

	pos->Buffer = new char[sizeof(VVector)];
	scale->Buffer = new char[sizeof(VVector)];
	rot->Buffer = new char[sizeof(VQuat)];

	memcpy(pos->Buffer, &Position, sizeof(VVector));
	memcpy(scale->Buffer, &Scale, sizeof(VVector));
	memcpy(rot->Buffer, &Rotation, sizeof(VQuat));

	res->Properties["Position"] = pos;
	res->Properties["Scale"] = scale;
	res->Properties["Rotation"] = rot;
	res->Properties["Color"] = color;
	res->Properties["Strength"] = strength;

	return res;
}

void VolumeRaytracer::Scene::VLight::Deserialize(std::shared_ptr<VSerializationArchive> archive)
{
	Position = archive->Properties["Position"]->To<VVector>();
	Scale = archive->Properties["Scale"]->To<VVector>();
	Rotation = archive->Properties["Rotation"]->To<VQuat>();
	Color = archive->Properties["Color"]->To<VColor>();
	IlluminationStrength = archive->Properties["Strength"]->To<float>();
}

void VolumeRaytracer::Scene::VLight::Initialize()
{
	
}

void VolumeRaytracer::Scene::VLight::BeginDestroy()
{
	
}
