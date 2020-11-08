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
#include <type_traits>

namespace VolumeRaytracer
{
	template<typename T>
	using VObjectPtr = std::shared_ptr<T>;

	class VObject
	{
	public:
		virtual ~VObject();

		template<typename T, class... _Types>
		static std::shared_ptr<T> CreateObject(_Types&&... args)
		{
			static_assert(std::is_base_of<VObject, T>::value, "T must inherit from VObject");

			std::shared_ptr<T> res(new T(std::forward<_Types>(args)...), [](T* ptr) {
				DestroyObject(dynamic_cast<VObject*>(ptr));
			});

			VObject* objectCast = dynamic_cast<VObject*>(res.get());

			if (objectCast != nullptr)
			{
				objectCast->Initialize();

				if (objectCast->CanEverTick())
				{
					objectCast->RegisterWithTickManager();
				}
			}

			return res;
		}

	private:
		static void DestroyObject(VObject* obj)
		{
			if (obj != nullptr)
			{
				if (obj->CanEverTick())
				{
					obj->UnregisterFromTickManager();
				}

				obj->BeginDestroy();
				delete obj;
			}
		}

	public:
		virtual void Tick(const float& deltaSeconds) {}

		virtual const bool CanEverTick() const { return false; }
		virtual bool ShouldTick() const { return false; }

	protected:
		virtual void Initialize() = 0;
		virtual void BeginDestroy() = 0;

	private:
		void RegisterWithTickManager();
		void UnregisterFromTickManager();
	};
}