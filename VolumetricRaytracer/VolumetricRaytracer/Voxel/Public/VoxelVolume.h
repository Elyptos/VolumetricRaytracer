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

#pragma once

#include "Octree.h"
#include "Object.h"
#include "ISerializable.h"
#include "Material.h"
#include "AABB.h"
#include <string>
#include <iterator>

namespace VolumeRaytracer
{
	class VTextureCube;

	namespace Scene
	{
		class VCamera;
		class VLight;
	}

	namespace Voxel
	{
		struct VVoxelIteratorElement
		{
		public:
			VVoxel Voxel;
			VIntVector Index3D;
			size_t Index;
		};

		class VVoxelVolume : public VObject, public IVSerializable
		{
		private:
			class VVoxelVolumeIterator : public std::iterator<std::output_iterator_tag, VVoxelIteratorElement>
			{
			public:
				explicit VVoxelVolumeIterator(VVoxelVolume& scene, size_t index = 0);

				VVoxelIteratorElement operator*() const;
				VVoxelVolumeIterator& operator++();
				VVoxelVolumeIterator operator++(int);
				bool operator!=(const VVoxelVolumeIterator& other) const;

			private:
				size_t Index = 0;
				VVoxelVolume& Volume;
			};

		public:
			VVoxelVolume(const uint8_t& resolution, const float& volumeExtends);

			unsigned int GetSize() const;
			size_t GetVoxelCount() const;
			float GetVolumeExtends() const;
			float GetCellSize() const;

			VAABB GetVolumeBounds() const;

			void SetVoxel(const VIntVector& voxelIndex, const VVoxel& voxel);
			VVoxel GetVoxel(const VIntVector& voxelIndex) const;
			VVoxel GetVoxel(const size_t& voxelIndex) const;
			bool IsValidVoxelIndex(const VIntVector& voxelIndex) const;

			void SetMaterial(const VMaterial& material);
			VMaterial GetMaterial() const;

			VVoxelVolumeIterator begin();
			VVoxelVolumeIterator end();


			void PostRender() override;


			const bool CanEverTick() const override;


			bool ShouldTick() const override;

			void MakeDirty();
			bool IsDirty() const;

			VVector VoxelIndexToRelativePosition(const VIntVector& voxelIndex) const;
			VIntVector RelativePositionToCellIndex(const VVector& pos) const;
			VIntVector RelativePositionToVoxelIndex(const VVector& pos) const;

			std::shared_ptr<VSerializationArchive> Serialize() const override;


			void Deserialize(std::shared_ptr<VSerializationArchive> archive) override;

			void SimplifyVolume();
			void GetGPUOctreeStructure(std::vector<VCellGPUOctreeNode>& outNodes, size_t& outNodeAxisCount) const;

			uint8_t GetResolution() const;

		protected:
			void Initialize() override;
			void BeginDestroy() override;

			void ClearDirtyFlag();

		private:
			float VolumeExtends = 0;
			float CellSize = 0;
			
			VCellOctree* Octree = nullptr; 

			VMaterial GeometryMaterial;

			bool DirtyFlag = false;
		};
	}
}