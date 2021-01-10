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

#include "VoxelVolume.h"
#include "MathHelpers.h"
#include <cmath>

VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolume(const uint8_t& resolution, const float& volumeExtends) :
	VolumeExtends(volumeExtends)
{
	Octree = new VCellOctree(resolution, VVoxel());
	Octree->ClearTreeAndSubdivideAll();

	CellSize = (volumeExtends * 2) / (Octree->GetVoxelCountAlongAxis() - 1.f);
}

unsigned int VolumeRaytracer::Voxel::VVoxelVolume::GetSize() const
{
	return Octree->GetVoxelCountAlongAxis();
}

size_t VolumeRaytracer::Voxel::VVoxelVolume::GetVoxelCount() const
{
	return Octree->GetVoxelCount();
}

float VolumeRaytracer::Voxel::VVoxelVolume::GetVolumeExtends() const
{
	return VolumeExtends;
}

float VolumeRaytracer::Voxel::VVoxelVolume::GetCellSize() const
{
	return CellSize;
}

VolumeRaytracer::VAABB VolumeRaytracer::Voxel::VVoxelVolume::GetVolumeBounds() const
{
	VAABB res;

	res.SetCenterPosition(VVector::ZERO);
	res.SetExtends(VVector::ONE * VolumeExtends);

	return res;
}

void VolumeRaytracer::Voxel::VVoxelVolume::SetVoxel(const VIntVector& voxelIndex, const VVoxel& voxel)
{
	Octree->SetVoxel(voxelIndex, voxel);
}

VolumeRaytracer::Voxel::VVoxel VolumeRaytracer::Voxel::VVoxelVolume::GetVoxel(const VIntVector& voxelIndex) const
{
	return Octree->GetVoxel(voxelIndex);
}

VolumeRaytracer::Voxel::VVoxel VolumeRaytracer::Voxel::VVoxelVolume::GetVoxel(const size_t& voxelIndex) const
{
	return Octree->GetVoxel(VMathHelpers::Index1DTo3D(voxelIndex, GetSize(), GetSize()));
}

bool VolumeRaytracer::Voxel::VVoxelVolume::IsValidVoxelIndex(const VIntVector& voxelIndex) const
{
	return	voxelIndex.X >= 0 && voxelIndex.X < GetSize() &&
			voxelIndex.Y >= 0 && voxelIndex.Y < GetSize() &&
			voxelIndex.Z >= 0 && voxelIndex.Z < GetSize();
}

//VolumeRaytracer::VVector VolumeRaytracer::Voxel::VVoxelVolume::VoxelIndexToWorldPosition(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const
//{
//	VVector volumeOrigin = VVector::ONE * -VolumeExtends;
//
//	return VVector(xPos, yPos, zPos) * CellSize + volumeOrigin;
//}

void VolumeRaytracer::Voxel::VVoxelVolume::SetMaterial(const VMaterial& material)
{
	GeometryMaterial = material;
}

VolumeRaytracer::VMaterial VolumeRaytracer::Voxel::VVoxelVolume::GetMaterial() const
{
	return GeometryMaterial;
}

VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator VolumeRaytracer::Voxel::VVoxelVolume::begin()
{
	return VVoxelVolumeIterator(*this, 0);
}

VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator VolumeRaytracer::Voxel::VVoxelVolume::end()
{
	return VVoxelVolumeIterator(*this, GetVoxelCount());
}

void VolumeRaytracer::Voxel::VVoxelVolume::PostRender()
{
	ClearDirtyFlag();
}

const bool VolumeRaytracer::Voxel::VVoxelVolume::CanEverTick() const
{
	return true;
}

bool VolumeRaytracer::Voxel::VVoxelVolume::ShouldTick() const
{
	return true;
}

void VolumeRaytracer::Voxel::VVoxelVolume::MakeDirty()
{
	DirtyFlag = true;
}

bool VolumeRaytracer::Voxel::VVoxelVolume::IsDirty() const
{
	return DirtyFlag;
}

VolumeRaytracer::VVector VolumeRaytracer::Voxel::VVoxelVolume::VoxelIndexToRelativePosition(const VIntVector& voxelIndex) const
{
	float distanceBetweenVoxel = GetCellSize();
	VVector voxelIndexF = VVector(voxelIndex.X, voxelIndex.Y, voxelIndex.Z);
	VVector volumeOrigin = -VVector::ONE * GetVolumeExtends();

	return voxelIndexF * distanceBetweenVoxel + volumeOrigin;
}

VolumeRaytracer::VIntVector VolumeRaytracer::Voxel::VVoxelVolume::RelativePositionToCellIndex(const VVector& pos) const
{
	VVector volumeOrigin = -VVector::ONE * GetVolumeExtends();
	VVector relativeVolumePos = pos - volumeOrigin;
	float distanceBetweenVoxel = GetCellSize();

	VIntVector res;

	res.X = std::floor(relativeVolumePos.X / distanceBetweenVoxel);
	res.Y = std::floor(relativeVolumePos.Y / distanceBetweenVoxel);
	res.Z = std::floor(relativeVolumePos.Z / distanceBetweenVoxel);

	return res;
}

VolumeRaytracer::VIntVector VolumeRaytracer::Voxel::VVoxelVolume::RelativePositionToVoxelIndex(const VVector& pos) const
{
	VVector volumeOrigin = -VVector::ONE * GetVolumeExtends();
	VVector relativeVolumePos = pos - volumeOrigin;
	float distanceBetweenVoxel = GetCellSize();

	VIntVector res;

	res.X = std::round(relativeVolumePos.X / distanceBetweenVoxel);
	res.Y = std::round(relativeVolumePos.Y / distanceBetweenVoxel);
	res.Z = std::round(relativeVolumePos.Z / distanceBetweenVoxel);

	return res;
}

std::shared_ptr<VolumeRaytracer::VSerializationArchive> VolumeRaytracer::Voxel::VVoxelVolume::Serialize() const
{
	std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();
	//res->BufferSize = GetVoxelCount() * sizeof(VVoxel);
	//res->Buffer = new char[res->BufferSize];

	//memcpy(res->Buffer, VoxelArr, res->BufferSize);

	//std::shared_ptr<VSerializationArchive> size = std::make_shared<VSerializationArchive>();
	//std::shared_ptr<VSerializationArchive> extends = std::make_shared<VSerializationArchive>();

	//size->BufferSize = sizeof(unsigned int);
	//extends->BufferSize = sizeof(float);

	//size->Buffer = new char[sizeof(unsigned int)];
	//extends->Buffer = new char[sizeof(float)];

	//memcpy(size->Buffer, &Size, sizeof(unsigned int));
	//memcpy(extends->Buffer, &VolumeExtends, sizeof(float));

	//res->Properties["Size"] = size;
	//res->Properties["Extends"] = extends;

	return res;
}

void VolumeRaytracer::Voxel::VVoxelVolume::Deserialize(std::shared_ptr<VSerializationArchive> archive)
{
	//memcpy(&Size, archive->Properties["Size"]->Buffer, sizeof(unsigned int));
	//memcpy(&VolumeExtends, archive->Properties["Extends"]->Buffer, sizeof(float));

	//if (VoxelArr != nullptr)
	//{
	//	delete[] VoxelArr;
	//}

	//VoxelArr = new VVoxel[GetVoxelCount()];
	//memcpy(VoxelArr, archive->Buffer, GetVoxelCount() * sizeof(VVoxel));

	//CellSize = (VolumeExtends * 2) / ((float)Size - 1.f);

	MakeDirty();
}

void VolumeRaytracer::Voxel::VVoxelVolume::SimplifyVolume()
{
	Octree->CollapseTree();
}

void VolumeRaytracer::Voxel::VVoxelVolume::GetGPUOctreeStructure(std::vector<VCellGPUOctreeNode>& outNodes, size_t& outNodeAxisCount) const
{
	Octree->GetGPUOctreeStructure(outNodes, outNodeAxisCount);
}

uint8_t VolumeRaytracer::Voxel::VVoxelVolume::GetResolution() const
{
	return Octree->GetMaxDepth();
}

void VolumeRaytracer::Voxel::VVoxelVolume::Initialize()
{
	//if (GetSize() > 0)
	//{
	//	VoxelArr = new VVoxel[GetVoxelCount()];

	//	for (unsigned int i = 0; i < GetVoxelCount(); i++)
	//	{
	//		VoxelArr[i].Material = 0u;
	//		VoxelArr[i].Density = 1;
	//	}

	//	MakeDirty();
	//}
	//else
	//{
	//	VoxelArr = nullptr;
	//}

	MakeDirty();
}

void VolumeRaytracer::Voxel::VVoxelVolume::BeginDestroy()
{
	if (Octree != nullptr)
	{
		delete Octree;
		Octree = nullptr;
	}
}

void VolumeRaytracer::Voxel::VVoxelVolume::ClearDirtyFlag()
{
	DirtyFlag = false;
}

VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator::VVoxelVolumeIterator(VVoxelVolume& scene, size_t index /*= 0*/)
	:Volume(scene),
	Index(index)
{

}

VolumeRaytracer::Voxel::VVoxelIteratorElement VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator::operator*() const
{
	if (Index < 0 || Index >= Volume.GetVoxelCount())
	{
		return VVoxelIteratorElement();
	}

	VVoxelIteratorElement elem;

	elem.Index3D = VMathHelpers::Index1DTo3D(Index, Volume.GetSize(), Volume.GetSize());
	elem.Voxel = Volume.Octree->GetVoxel(elem.Index3D);
	elem.Index = Index;
	

	return elem;
}

VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator& VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator::operator++()
{
	++Index;
	return *this;
}

bool VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator::operator!=(const VVoxelVolumeIterator& other) const
{
	return Index != other.Index;
}

VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator VolumeRaytracer::Voxel::VVoxelVolume::VVoxelVolumeIterator::operator++(int)
{
	VVoxelVolumeIterator res(*this);
	operator++();

	return res;
}
