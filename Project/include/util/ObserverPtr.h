#pragma once

#include <cassert>
#include <memory>

namespace fig
{
	// Explicit non-owning pointer

	template <typename T>
	struct observer_ptr
	{
		constexpr observer_ptr() : _ptr(nullptr)
		{
		}

		constexpr observer_ptr(T* ptr) : _ptr(ptr)
		{
		}

		T* operator->() const { return _ptr; }
		T& operator*() const { return *_ptr; }
		operator T* () const { return _ptr; }

		T* get() const { return _ptr; }
		explicit operator bool() const { return _ptr != nullptr; }

		template <typename U> 
			requires std::derived_from<U, T>
		observer_ptr<U> as() const
		{
			U* pDerived = dynamic_cast<U*>(_ptr);
			assert(pDerived != nullptr);
			return pDerived;
		}

		template <typename U>
			requires std::derived_from<T, U> and (not std::same_as<T, U>)
		operator observer_ptr<U>() const { return _ptr; }

		void reset(T* ptr = nullptr) { _ptr = ptr; }

	private:
		T* _ptr;
	};
}