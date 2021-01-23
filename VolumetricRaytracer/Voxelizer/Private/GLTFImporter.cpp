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

#include "GLTFImporter.h"
#include "SceneInfo.h"
#include <GLTFSDK/Document.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <iostream>

std::shared_ptr<VolumeRaytracer::Voxelizer::VSceneInfo> VolumeRaytracer::Voxelizer::VGLTFImporter::ImportScene(const Microsoft::glTF::Document* document, const Microsoft::glTF::GLTFResourceReader* resourceReader)
{
	std::shared_ptr<VSceneInfo> sceneInfo = std::make_shared<VSceneInfo>();

	std::cout << "[INFO] Importing meshes" << std::endl;

	for (size_t i = 0; i < document->meshes.Size(); i++)
	{
		const Microsoft::glTF::Mesh& mesh = document->meshes[i];

		std::cout << "[INFO] Importing mesh: " << mesh.name << std::endl;

		VMeshInfo meshInfo;
		meshInfo.MeshName = mesh.name;

		for (auto& primitive : mesh.primitives)
		{
			std::string positionAccessorID;
			std::string normalAccessorID;

			if (primitive.TryGetAttributeAccessorId(Microsoft::glTF::ACCESSOR_POSITION, positionAccessorID)
				&& primitive.TryGetAttributeAccessorId(Microsoft::glTF::ACCESSOR_NORMAL, normalAccessorID))
			{
				if (document->accessors.Has(primitive.indicesAccessorId)
					&& document->accessors.Has(positionAccessorID)
					&& document->accessors.Has(normalAccessorID))
				{
					const Microsoft::glTF::Accessor& indicesAcc = document->accessors[primitive.indicesAccessorId];
					const Microsoft::glTF::Accessor& positionAcc = document->accessors[positionAccessorID];
					const Microsoft::glTF::Accessor& normalAcc = document->accessors[normalAccessorID];

					VVector volumeOffset = VVector::ZERO;

					if (positionAcc.min.size() >= 3 && positionAcc.max.size() >= 3)
					{
						VVector min = VVector(positionAcc.min[0], positionAcc.min[1], positionAcc.min[2]) * 100.f;
						VVector max = VVector(positionAcc.max[0], positionAcc.max[1], positionAcc.max[2]) * 100.f;

						VVector extends = ((max - min) * 0.5f);

						volumeOffset = max - extends;

						meshInfo.Bounds = VAABB(volumeOffset, extends + VVector::ONE * 5.f);
					}
					else
					{
						std::cout << "[WARNING] No bounds found for primitive!" << std::endl;
					}

					if (indicesAcc.componentType == Microsoft::glTF::COMPONENT_UNSIGNED_SHORT)
					{
						std::vector<unsigned short> indices = resourceReader->ReadBinaryData<unsigned short>(*document, indicesAcc);

						for (auto& index : indices)
						{
							meshInfo.Indices.push_back(index);
						}
					}
					else if (indicesAcc.componentType == Microsoft::glTF::COMPONENT_UNSIGNED_INT)
					{
						std::vector<unsigned int> indices = resourceReader->ReadBinaryData<unsigned int>(*document, indicesAcc);

						for (auto& index : indices)
						{
							meshInfo.Indices.push_back(index);
						}
					}
					else
					{
						std::cerr << "[ERROR] Unsupported indices format!" << std::endl;
						continue;
					}

					if (positionAcc.componentType == Microsoft::glTF::COMPONENT_FLOAT && normalAcc.componentType == Microsoft::glTF::COMPONENT_FLOAT)
					{
						std::vector<float> positions = resourceReader->ReadBinaryData<float>(*document, positionAcc);
						std::vector<float> normals = resourceReader->ReadBinaryData<float>(*document, normalAcc);

						if (positions.size() != normals.size())
						{
							std::cerr << "[ERROR] Vertex and normal data are not the same size!" << std::endl;
							continue;
						}
						
						if (positions.size() % 3 != 0 || normals.size() % 3 != 0)
						{
							std::cerr << "[ERROR] Invalid vector format. Only three component vectors are supported!" << std::endl;
							continue;
						}

						for (size_t compIndex = 0; compIndex < positions.size(); compIndex += 3)
						{
							VVertex vertex;

							vertex.Position = VVector(positions[compIndex], positions[compIndex + 1], positions[compIndex + 2]) * 100.f;
							vertex.Position = vertex.Position - volumeOffset;

							vertex.Normal = VVector(normals[compIndex], normals[compIndex + 1], normals[compIndex + 2]);

							meshInfo.Vertices.push_back(vertex);
						}
					}
					else
					{
						std::cerr << "[ERROR] Unsupported vertex format!" << std::endl;
						continue;
					}
				}
				else
				{
					std::cerr << "[ERROR] Invalid accessor data inside gltf file. File may be corrupted!" << std::endl;
				}
			}
			else
			{
				std::cout << "[WARNING] Invalid mesh primtive detected. Either no vertices or normals." << std::endl;
			}
		}

		if (meshInfo.Indices.size() == 0)
		{
			std::cout << "[WARNING] Mesh has no index data, skipping." << std::endl;
			continue;
		}

		if (meshInfo.Vertices.size() == 0)
		{
			std::cout << "[WARNING] Mesh has no vertices, skipping." << std::endl;
			continue;
		}

		sceneInfo->Meshes[mesh.id] = meshInfo;
	}

	std::cout << "[INFO] Importing objects with meshes" << std::endl;

	for (size_t i = 0; i < document->nodes.Size(); i++)
	{
		const Microsoft::glTF::Node& node = document->nodes[i];

		std::cout << "[INFO] Trying to import object: " << node.name << std::endl;

		if (!node.IsEmpty() && node.HasValidTransformType() && sceneInfo->Meshes.find(node.meshId) != sceneInfo->Meshes.end())
		{
			VObjectInfo objectInfo;

			objectInfo.MeshID = node.meshId;
			objectInfo.Position = VVector(node.translation.x, node.translation.y, node.translation.z) * 100.f;
			objectInfo.Scale = VVector(node.scale.x, node.scale.y, node.scale.z);
			objectInfo.Rotation = VQuat(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w);

			sceneInfo->Objects.push_back(objectInfo);
		}
		else
		{
			std::cout << "[INFO] Skipping non geometry object." << std::endl;
		}
	}

	return sceneInfo;
}
