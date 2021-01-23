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
#include <boost/unordered_map.hpp>
#include <string>

namespace VolumeRaytracer
{
	struct VSerializationArchive
	{
	public:
		VSerializationArchive() = default;
		~VSerializationArchive()
		{
			if (Buffer != nullptr)
			{
				delete[] Buffer;
				Buffer = nullptr;
			}
		}

		template<typename T> 
		static std::shared_ptr<VSerializationArchive> From(const T* src)
		{
			std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();

			res->BufferSize = sizeof(T);
			res->Buffer = new char[res->BufferSize];

			memcpy(res->Buffer, src, res->BufferSize);

			return res;
		}

		template<typename T>
		T To()
		{
			T res;

			if (Buffer != nullptr && BufferSize >= sizeof(T))
			{
				memcpy(&res, Buffer, BufferSize);
			}

			return res;
		}

	public:
		size_t BufferSize = 0;
		char* Buffer = nullptr;

		boost::unordered_map<std::string, std::shared_ptr<VSerializationArchive>> Properties;
	};

	class IVSerializable
	{
	public:
		virtual std::shared_ptr<VSerializationArchive> Serialize() const = 0;
		virtual void Deserialize(std::shared_ptr<VSerializationArchive> archive) = 0;
	};
}