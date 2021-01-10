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

#include "Voxel.h"

bool VolumeRaytracer::Voxel::VCell::HasSurface() const
{
	int v0Sign = VMathHelpers::Sign(Voxels[0].Density);

	bool sameSign = (
		v0Sign == VMathHelpers::Sign(Voxels[1].Density) &&
		v0Sign == VMathHelpers::Sign(Voxels[2].Density) &&
		v0Sign == VMathHelpers::Sign(Voxels[3].Density) &&
		v0Sign == VMathHelpers::Sign(Voxels[4].Density) &&
		v0Sign == VMathHelpers::Sign(Voxels[5].Density) &&
		v0Sign == VMathHelpers::Sign(Voxels[6].Density) &&
		v0Sign == VMathHelpers::Sign(Voxels[7].Density)
	);

	bool sameMaterial = (Voxels[0].Material &
		Voxels[1].Material &
		Voxels[2].Material &
		Voxels[3].Material &
		Voxels[4].Material &
		Voxels[5].Material &
		Voxels[6].Material &
		Voxels[7].Material) == Voxels[0].Material;

	return !(sameSign && sameMaterial);
}

void VolumeRaytracer::Voxel::VCell::FillWithVoxel(const VVoxel& voxel)
{
	Voxels[0] = voxel;
	Voxels[1] = voxel;
	Voxels[2] = voxel;
	Voxels[3] = voxel;
	Voxels[4] = voxel;
	Voxels[5] = voxel;
	Voxels[6] = voxel;
	Voxels[7] = voxel;
}

uint8_t VolumeRaytracer::Voxel::VCell::GetVoxelIndex(const VIntVector& index3D)
{
	assert(index3D.X >= 0 && index3D.X <= 1 && index3D.Y >= 0 && index3D.Y <= 1 && index3D.Z >= 0 && index3D.Z <= 1);

	uint8_t res = index3D.X;

	res |= (index3D.Y << 1);
	res |= (index3D.Z << 2);

	return res;
}

const VolumeRaytracer::VIntVector VolumeRaytracer::Voxel::VCell::VOXEL_COORDS[8] = {
	VIntVector(0,0,0),
	VIntVector(1,0,0),
	VIntVector(0,1,0),
	VIntVector(1,1,0),
	VIntVector(0,0,1),
	VIntVector(1,0,1),
	VIntVector(0,1,1),
	VIntVector(1,1,1)
};

VolumeRaytracer::Voxel::VVoxel VolumeRaytracer::Voxel::VCell::GetAvgVoxel() const
{
	VVoxel res;

	res.Material = Voxels[0].Material;

	for (int i = 0; i < 8; i++)
	{
		res.Density += Voxels[i].Density;
	}

	res.Density /= 8.f;

	return res;
}

const float VolumeRaytracer::Voxel::VVoxel::DEFAULT_DENSITY = 30.f;
