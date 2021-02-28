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

#include "SpotLight.h"

std::shared_ptr<VolumeRaytracer::VSerializationArchive> VolumeRaytracer::Scene::VSpotLight::Serialize() const
{
	std::shared_ptr<VSerializationArchive> res = VLight::Serialize();

	res->Properties["AttL"] = VSerializationArchive::From<float>(&AttenuationLinear);
	res->Properties["AttExp"] = VSerializationArchive::From<float>(&AttenuationExp);
	res->Properties["AngleF"] = VSerializationArchive::From<float>(&FalloffAngle);
	res->Properties["Angle"] = VSerializationArchive::From<float>(&Angle);

	return res;
}

void VolumeRaytracer::Scene::VSpotLight::Deserialize(const std::wstring& sourcePath, std::shared_ptr<VSerializationArchive> archive)
{
	VLight::Deserialize(sourcePath, archive);

	AttenuationLinear = archive->Properties["AttL"]->To<float>();
	AttenuationExp = archive->Properties["AttExp"]->To<float>();
	FalloffAngle = archive->Properties["AngleF"]->To<float>();
	Angle = archive->Properties["Angle"]->To<float>();
}
