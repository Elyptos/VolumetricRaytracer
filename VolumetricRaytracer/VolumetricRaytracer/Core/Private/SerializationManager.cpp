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

#include "SerializationManager.h"
#include "ISerializable.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace VolumeRaytracer
{
	namespace Internal
	{
		void SerializeArchive(const std::shared_ptr<VSerializationArchive>& archive, std::ofstream& stream)
		{
			stream.write(reinterpret_cast<char*>(&archive->BufferSize), sizeof(size_t));

			if(archive->BufferSize > 0)
				stream.write(archive->Buffer, archive->BufferSize);

			size_t numProps = archive->Properties.size();

			stream.write(reinterpret_cast<char*>(&numProps), sizeof(size_t));
			
			for (const auto& elem : archive->Properties)
			{
				const std::string& propName = elem.first;

				size_t numChars = propName.size() + 1;

				stream.write(reinterpret_cast<char*>(&numChars), sizeof(size_t));
				stream.write(propName.c_str(), numChars);

				SerializeArchive(elem.second, stream);
			}
		}

		std::shared_ptr<VSerializationArchive> DeserializeArchive(std::ifstream& stream)
		{
			std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();

			char* buffer = new char[sizeof(size_t)];

			stream.read(buffer, sizeof(size_t));

			memcpy(&res->BufferSize, buffer, sizeof(size_t));

			delete[] buffer;

			if (res->BufferSize > 0)
			{
				res->Buffer = new char[res->BufferSize];
				stream.read(res->Buffer, res->BufferSize);
			}

			size_t numProperties = 0;

			buffer = new char[sizeof(size_t)];
			stream.read(buffer, sizeof(size_t));

			memcpy(&numProperties, buffer, sizeof(size_t));

			delete[] buffer;

			for (size_t i = 0; i < numProperties; i++)
			{
				buffer = new char[sizeof(size_t)];

				size_t strLength = 0;

				stream.read(buffer, sizeof(size_t));
				memcpy(&strLength, buffer, sizeof(size_t));

				delete[] buffer;
				buffer = new char[strLength];

				stream.read(buffer, strLength);

				std::string propertyName = std::string(buffer);

				delete[] buffer;
				
				std::shared_ptr<VSerializationArchive> propArchive = DeserializeArchive(stream);

				res->Properties[propertyName] = propArchive;
			}

			return res;
		}
	}
}

bool VolumeRaytracer::VSerializationManager::LoadObjectFromFile(std::shared_ptr<IVSerializable> obj, const std::string& filePath)
{
	if (boost::filesystem::exists(filePath))
	{
		std::ifstream stream = std::ifstream(filePath, std::ios::binary);

		std::shared_ptr<VSerializationArchive> archive = Internal::DeserializeArchive(stream);
		obj->Deserialize(archive);

		stream.close();

		return true;
	}
	else
	{
		return false;
	}
}

void VolumeRaytracer::VSerializationManager::SaveToFile(VObjectPtr<VObject> object, const std::string& filePath)
{
	if (object != nullptr)
	{
		std::shared_ptr<IVSerializable> serializable = std::dynamic_pointer_cast<IVSerializable>(object);

		if (serializable != nullptr)
		{
			std::shared_ptr<VSerializationArchive> archive = serializable->Serialize();

			std::ofstream stream = std::ofstream(filePath, std::ios::binary);

			Internal::SerializeArchive(archive, stream);

			stream.close();
		}
	}
}
