#pragma once

namespace VolumeRaytracer
{
	class VObject
	{
	public:
		virtual ~VObject();

		template<typename T, class... _Types>
		static T* CreateObject(_Types&&... args)
		{
			T* res = new T(std::forward<_Types>(args)...);

			VObject* objectCast = dynamic_cast<VObject*>(res);

			if (objectCast != nullptr)
			{
				objectCast->Initialize();
			}

			return res;
		}

		static void DestroyObject(VObject* obj)
		{
			if (obj != nullptr)
			{
				obj->BeginDestroy();
				delete obj;
			}
		}

	protected:
		virtual void Initialize() = 0;
		virtual void BeginDestroy() = 0;
		virtual void Tick(const float& deltaSeconds) = 0;

		virtual bool CanEverTick() const = 0;
		virtual bool ShouldTick() const = 0;
	};

	template<typename T, class... _Types>
	class VObjectPtr
	{
	private:
		class VObjectPtrCounter
		{
		public:
			VObjectPtrCounter()
				:Counter(0){};

			VObjectPtrCounter(const VObjectPtrCounter& other) = delete;
			VObjectPtrCounter& operator=(const VObjectPtrCounter& other) = delete;

			~VObjectPtrCounter() {}

			void Reset()
			{
				Counter = 0;
			}

			unsigned int GetCounter()
			{
				return Counter;
			}

			void operator++()
			{
				Counter++;
			}

			void operator++(int)
			{
				Counter++;
			}

			void operator--()
			{
				Counter--;
			}

			void operator--(int)
			{
				Counter--;
			}

		private:
			unsigned int Counter;
		};

	public:
		VObjectPtr(_Types&&... args)
		{
			Counter = new VObjectPtrCounter();
			Ptr = VObject::CreateObject<T>(std::forward<_Types>(args)...);

			(*Counter)++;
		}

		VObjectPtr(T* ptr)
		{
			Ptr = ptr;
			Counter = new VObjectPtrCounter();

			(*Counter)++;
		}

		VObjectPtr(VObjectPtr<T>& other)
		{
			Ptr = other.Ptr;
			Counter = other.Counter;

			if(IsValid())
				(*Counter)++;
		}

		VObjectPtr& operator=(const VObjectPtr& other)
		{
			Ptr = other.Ptr;
			Counter = other.Counter;

			if(IsValid())
				(*Counter)++;
		}

		~VObjectPtr()
		{
			if (IsValid())
			{
				(*Counter)--;

				if (Counter->GetCounter() <= 0)
				{
					delete Counter;
					
					VObject* obj = dynamic_cast<VObject*>(Ptr);

					if (obj != nullptr)
					{
						VObject::DestroyObject(obj);
					}
					else
					{
						delete Ptr;
					}

					Counter = nullptr;
					Ptr = nullptr;
				}
			}
		}

		bool IsValid()
		{
			return Ptr != nullptr && Counter != nullptr;
		}

		T* Get()
		{
			return Ptr;
		}

		T& operator*()
		{
			return *Ptr;
		}

		T* operator->()
		{
			return Ptr;
		}

	private:
		T* Ptr = nullptr;
		VObjectPtrCounter* Counter = nullptr;
	};
}