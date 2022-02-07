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
#include "Object.h"
#include <string>

namespace VolumeRaytracer
{
	class IVSerializable;

	class VSerializationManager
	{
	private:
		static bool LoadObjectFromFile(std::shared_ptr<IVSerializable> obj, const std::wstring& filePath);

	public:
		template<typename T>
		static VObjectPtr<T> LoadFromFile(const std::wstring& filePath)
		{
			VObjectPtr<T> obj = VObject::CreateObject<T>();

			std::shared_ptr<IVSerializable> serializable = std::dynamic_pointer_cast<IVSerializable>(obj);

			if (serializable != nullptr && LoadObjectFromFile(serializable, filePath))
			{
				return obj;
			}
			else
			{
				return nullptr;
			}
		}

		static void SaveToFile(VObjectPtr<VObject> object, const std::string& filePath);
	};
}