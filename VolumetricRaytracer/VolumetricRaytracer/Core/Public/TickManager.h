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
#include <list>

namespace VolumeRaytracer
{
	class VObject;

	class VGlobalTickManager
	{
	public:
		void AddTickableObject(VObject* obj);
		void RemoveTickableObject(VObject* obj);
		void CallTickOnAllAllowedObjects(const float& deltaTime);
		void CallPostRenderOnAllAllowedObjects();

		bool IsAllowedToAddObject(VObject* obj) const;

		static VGlobalTickManager& Instance()
		{
			static VGlobalTickManager instance;

			return instance;
		}

	private:
		std::list<VObject*> TickableObjects;
	};
}