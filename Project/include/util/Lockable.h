#pragma once

#include <functional>
#include <mutex>

template <typename T>
concept Lockable = requires (T mut)
{
	mut.try_lock();
	mut.lock();
	mut.unlock();
};

template <Lockable... MutexTypes>
static void LockAndDo(std::function<void()> fn, MutexTypes&... mutexes)
{
	std::scoped_lock _ { mutexes... };
	fn();
}

template <typename ReturnType, Lockable MutexType>
[[nodiscard]] static ReturnType LockAndReturn(std::function<ReturnType()> fn, MutexType& mutex)
{
	std::scoped_lock _ { mutex };
	return fn();
}
