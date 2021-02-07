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
#include "Color.h"
#include "ISerializable.h"

namespace VolumeRaytracer
{
	namespace Scene
	{
		class VLight : public VLevelObject, public IVSerializable
		{
		public:
			float IlluminationStrength = 1.f;
			VColor Color = VColor::WHITE;

		protected:
			void Initialize() override;


			void BeginDestroy() override;


			std::shared_ptr<VSerializationArchive> Serialize() const override;
			void Deserialize(std::shared_ptr<VSerializationArchive> archive) override;
		};
	}
}