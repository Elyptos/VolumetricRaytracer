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

#include "DensityGenerator.h"
#include "MathHelpers.h"

float VolumeRaytracer::Scene::VDensityShape::Evaluate(const VVector& worldPosition) const
{
	VVector samplePos = worldPosition - Position;

	samplePos = Rotation.Inverse() * samplePos;

	return EvaluateInternal(samplePos);
}

float VolumeRaytracer::Scene::VBox::EvaluateInternal(const VVector& p) const
{
	VVector q = p.Abs() - Extends;
	return VVector::Max(q, 0.f).Length() + VMathHelpers::Min(VMathHelpers::Max(q.X, VMathHelpers::Max(q.Y, q.Z)), 0.f);
}

float VolumeRaytracer::Scene::VSphere::EvaluateInternal(const VVector& p) const
{
	return p.Length() - Radius;
}

float VolumeRaytracer::Scene::VCylinder::EvaluateInternal(const VVector& p) const
{
	VVector d = VVector(VVector(p.X, p.Z, 0.f).Length(), p.Y, 0.f).Abs() - VVector(Radius, Height, 0.f);
	return VMathHelpers::Min(VMathHelpers::Max(d.X, d.Y), 0.f) + VVector::Max(d, 0.f).Length();
}


float VolumeRaytracer::Scene::VDensityShapeContainer::Evaluate(const VVector& p) const
{
	float d = 0.f;

	VVector samplePos = p;

	if (!Shape.expired())
	{
		d = Shape.lock()->Evaluate(p);

		samplePos = samplePos - Shape.lock()->Position;
		samplePos = Shape.lock()->Rotation.Inverse() * samplePos;
	}
	else if(Children.size() > 0)
	{
		d = Children[0].Evaluate(p);
	}

	for (unsigned int i = 0; i < Children.size(); i++)
	{
		const auto& child = Children[i];

		float childD = child.Evaluate(samplePos);

		switch (child.CombinationType)
		{
		case ECombinationType::ADD:
			d = VMathHelpers::Min(d, childD);
			break;
		case ECombinationType::SUBTRACT:
			d = VMathHelpers::Max(d, -childD);
			break;
		default:
			break;
		}
	}

	return d;
}

VolumeRaytracer::Scene::VDensityShapeContainer& VolumeRaytracer::Scene::VDensityShapeContainer::AddChild(std::weak_ptr<VDensityShape> shape, ECombinationType combinationType)
{
	VDensityShapeContainer container;

	container.Shape = shape;
	container.CombinationType = combinationType;

	Children.push_back(container);

	return Children[Children.size() - 1];
}

float VolumeRaytracer::Scene::VDensityGenerator::Evaluate(const VVector& worldPos) const
{
	VVector samplePos = worldPos - Position;
	samplePos = Rotation.Inverse() * samplePos;

	return Root.Evaluate(samplePos);
}

VolumeRaytracer::Scene::VDensityShapeContainer& VolumeRaytracer::Scene::VDensityGenerator::GetRootShape()
{
	return Root;
}

void VolumeRaytracer::Scene::VDensityGenerator::Initialize()
{
}

void VolumeRaytracer::Scene::VDensityGenerator::BeginDestroy()
{
}

