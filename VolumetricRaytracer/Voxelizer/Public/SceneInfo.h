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
#include "Vector.h"
#include "Quat.h"
#include "AABB.h"
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <Color.h>
#include "Material.h"

namespace VolumeRaytracer
{
	namespace Voxelizer
	{
		struct VMaterialTextures
		{
		public:
			VVector2D TextureTiling = VVector2D(100.f, 100.f);

			std::wstring Albedo;
			std::wstring Normal;
			std::wstring RM;
		};

		struct VTextureLibrary
		{
			boost::unordered_map<std::string, VMaterialTextures> Materials;
		};

		struct VVertex
		{
			VVector Position;
			VVector Normal;
		};

		struct VMeshInfo
		{
			std::string MeshName;
			std::vector<VVertex> Vertices;
			std::vector<size_t> Indices;
			VAABB Bounds;
			std::string MaterialName;
			VMaterial Material;
		};

		struct VObjectInfo
		{
			std::string MeshID;
			VVector Position;
			VVector Scale;
			VQuat Rotation;
		};

		enum class ELightType
		{
			DIRECTIONAL,
			POINT,
			SPOT
		};

		struct VLightInfo
		{
		public:
			VVector Position;
			VQuat Rotation;
			ELightType LightType;
			VColor Color = VColor::WHITE;
			float Intensity = 0.f;
			float AttL = 0.f;
			float AttExp = 0.f;
			float FalloffAngle = 0.f;
			float Angle = 0.f;
		};

		struct VSceneInfo
		{
			boost::unordered_map<std::string, VMeshInfo> Meshes;
			std::vector<VObjectInfo> Objects;
			std::vector<VLightInfo> Lights;
		};
	}
}