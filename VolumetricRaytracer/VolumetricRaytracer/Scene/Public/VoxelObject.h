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

#include "LevelObject.h"
#include "RenderableObject.h"
#include "ISerializable.h"

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VVoxelVolume;
	}

	namespace Scene
	{
		class VVoxelObject : public VLevelObject, public IVRenderableObject, public std::enable_shared_from_this<VVoxelObject>, public IVSerializable
		{
		public:
			void SetVoxelVolume(VObjectPtr<Voxel::VVoxelVolume> volume);
			std::weak_ptr<Voxel::VVoxelVolume> GetVoxelVolume() override;


			std::shared_ptr<VSerializationArchive> Serialize() const override;


			void Deserialize(std::shared_ptr<VSerializationArchive> archive) override;

		private:
			VObjectPtr<Voxel::VVoxelVolume> VoxelVolume;
		protected:
			void Initialize() override;

			void BeginDestroy() override;
		};
	}
}