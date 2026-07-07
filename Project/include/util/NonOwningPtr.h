#pragma once

#include <cassert>

namespace fig
{
	template <typename T>
	class non_owning_ptr
	{
	public:
		non_owning_ptr(T* ptr) : _ptr(ptr)
		{
		}

		T* operator->() const { return _ptr; }
		T& operator*() const { return *_ptr; }
		operator T* () const { return _ptr; }

		T* get() const { return _ptr; }
		explicit operator bool() const { return _ptr != nullptr; }

		template <typename U>
			requires std::derived_from<U, T>
		non_owning_ptr<U> As() const
		{
			U* pDerived = dynamic_cast<U*>(_ptr);
			assert(pDerived != nullptr);
			return pDerived;
		}

		template <typename U>
			requires std::derived_from<T, U>
		operator non_owning_ptr<U>() const { return _ptr; }

	private:
		T* _ptr;
	};

}