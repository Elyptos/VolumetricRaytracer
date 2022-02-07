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

#include <memory>
#include "SceneInfo.h"

namespace Microsoft
{
	namespace glTF
	{
		class Document;
		class GLTFResourceReader;
		struct Node;
	}
}

namespace VolumeRaytracer
{
	namespace Voxelizer
	{
		struct VSceneInfo;

		class VGLTFImporter
		{
		public:
			static std::shared_ptr<VSceneInfo> ImportScene(const Microsoft::glTF::Document* document, const Microsoft::glTF::GLTFResourceReader* resourceReader);

		private:
			static bool IsLight(const Microsoft::glTF::Node* node);
			static VLightInfo GetLightInfo(const Microsoft::glTF::Node* node);
		};
	}
}