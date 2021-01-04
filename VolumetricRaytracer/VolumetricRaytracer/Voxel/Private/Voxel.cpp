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
	bool sameSign = (VMathHelpers::Sign(Voxels[0].Density) &
		VMathHelpers::Sign(Voxels[1].Density) &
		VMathHelpers::Sign(Voxels[2].Density) &
		VMathHelpers::Sign(Voxels[3].Density) &
		VMathHelpers::Sign(Voxels[4].Density) &
		VMathHelpers::Sign(Voxels[5].Density) &
		VMathHelpers::Sign(Voxels[6].Density) &
		VMathHelpers::Sign(Voxels[7].Density)) == VMathHelpers::Sign(Voxels[0].Density);

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
