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

	//class VObjectPtrBase
	//{
	//protected:
	//	class VObjectPtrCounter
	//	{
	//	public:
	//		VObjectPtrCounter()
	//			:Counter(0) {};

	//		VObjectPtrCounter(const VObjectPtrCounter& other) = delete;
	//		VObjectPtrCounter& operator=(const VObjectPtrCounter& other) = delete;

	//		~VObjectPtrCounter() {}

	//		void Reset()
	//		{
	//			Counter = 0;
	//		}

	//		unsigned int GetCounter()
	//		{
	//			return Counter;
	//		}

	//		void operator++()
	//		{
	//			Counter++;
	//		}

	//		void operator++(int)
	//		{
	//			Counter++;
	//		}

	//		void operator--()
	//		{
	//			Counter--;
	//		}

	//		void operator--(int)
	//		{
	//			Counter--;
	//		}

	//	private:
	//		unsigned int Counter;
	//	};

	//protected:
	//	VObjectPtrCounter* Counter = nullptr;
	//};

	//template<typename T>
	//class VObjectPtr : public VObjectPtrBase
	//{
	//public:
	//	VObjectPtr()
	//	{}

	//	VObjectPtr(T* ptr)
	//	{
	//		Ptr = ptr;
	//		Counter = new VObjectPtrCounter();

	//		AddReference();
	//	}

	//	//template<typename U>
	//	//VObjectPtr(U* ptr)
	//	//{
	//	//	Ptr = ptr;
	//	//	Counter = new VObjectPtrCounter();

	//	//	AddReference();
	//	//}

	//	VObjectPtr(VObjectPtr<T>& other)
	//	{
	//		Ptr = other.Ptr;
	//		Counter = other.Counter;

	//		AddReference();
	//	}

	//	template<typename U>
	//	VObjectPtr(VObjectPtr& other)
	//	{
	//		Ptr = other.Get();
	//		Counter = other.Counter;

	//		AddReference();
	//	}

	//	template<class... _Types>
	//	static VObjectPtr<T> Create(_Types&&... args)
	//	{
	//		T* obj = VObject::CreateObject<T>(std::forward<_Types>(args)...);

	//		return VObjectPtr<T>(obj);
	//	}

	//	VObjectPtr& operator=(const VObjectPtr& other)
	//	{
	//		ReleaseReference();

	//		Ptr = other.Ptr;
	//		Counter = other.Counter;

	//		AddReference();

	//		return *this;
	//	}

	//	//template<typename U>
	//	//operator VObjectPtr<U> const
	//	//{
	//	//	return VObjectPtr<U>(*this);
	//	//}

	//	~VObjectPtr()
	//	{
	//		ReleaseReference();
	//	}

	//	bool IsValid()
	//	{
	//		return Ptr != nullptr && Counter != nullptr;
	//	}

	//	T* Get() const
	//	{
	//		return Ptr;
	//	}

	//	T& operator*()
	//	{
	//		return *Ptr;
	//	}

	//	T* operator->()
	//	{
	//		return Ptr;
	//	}

	//private:
	//	void ReleaseReference()
	//	{
	//		if (IsValid())
	//		{
	//			(*Counter)--;

	//			if (Counter->GetCounter() <= 0)
	//			{
	//				delete Counter;

	//				VObject* obj = dynamic_cast<VObject*>(Ptr);

	//				if (obj != nullptr)
	//				{
	//					VObject::DestroyObject(obj);
	//				}
	//				else
	//				{
	//					delete Ptr;
	//				}

	//				Counter = nullptr;
	//				Ptr = nullptr;
	//			}
	//		}
	//	}

	//	void AddReference()
	//	{
	//		if (IsValid())
	//		{
	//			(*Counter)++;
	//		}
	//	}

	//private:
	//	T* Ptr = nullptr;
	//};
}