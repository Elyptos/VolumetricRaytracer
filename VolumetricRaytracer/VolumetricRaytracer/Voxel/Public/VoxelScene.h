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

#include "Voxel.h"
#include "Object.h"
#include "AABB.h"
#include <string>
#include <iterator>

namespace VolumeRaytracer
{
	class VTextureCube;

	namespace Scene
	{
		class VCamera;
	}

	namespace Voxel
	{
		struct VVoxelIteratorElement
		{
		public:
			VVoxel Voxel;
			unsigned int X;
			unsigned int Y;
			unsigned int Z;
			unsigned int Index;
		};

		class VVoxelScene : public VObject 
		{
		private:
			class VVoxelSceneIterator : public std::iterator<std::output_iterator_tag, VVoxelIteratorElement>
			{
			public:
				explicit VVoxelSceneIterator(VVoxelScene& scene, size_t index = 0);

				VVoxelIteratorElement operator*() const;
				VVoxelSceneIterator& operator++();
				VVoxelSceneIterator operator++(int);
				bool operator!=(const VVoxelSceneIterator& other) const;

			private:
				size_t Index = 0;
				VVoxelScene& Scene;
			};

		public:
			VVoxelScene(const unsigned int& size, const float& volumeExtends);

			unsigned int GetSize() const;
			unsigned int GetVoxelCount() const;
			float GetVolumeExtends() const;
			float GetCellSize() const;

			VAABB GetSceneBounds() const;

			VObjectPtr<Scene::VCamera> GetSceneCamera() const;

			void SetVoxel(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos, const VVoxel& voxel);
			VVoxel GetVoxel(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const;
			bool IsValidVoxelIndex(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const;

			void SetEnvironmentTexture(VObjectPtr<VTextureCube> texture);
			VObjectPtr<VTextureCube> GetEnvironmentTexture() const;

			VVector VoxelIndexToWorldPosition(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const;

			VVoxelSceneIterator begin();
			VVoxelSceneIterator end();

		protected:
			void Initialize() override;
			void BeginDestroy() override;

		private:
			unsigned int Size = 0;
			float VolumeExtends = 0;
			float CellSize = 0;
			VVoxel* VoxelArr = nullptr;

			VObjectPtr<Scene::VCamera> Camera;

			VObjectPtr<VTextureCube> EnvironmentTexture;
		};
	}
}