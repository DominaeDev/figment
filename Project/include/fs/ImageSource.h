#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include <map>
#include <mutex>
#include <future>

namespace fig::io
{
	using ImagePromise = std::promise<fig::sdl::Surface>;
	using ImageFuture = std::future<fig::sdl::Surface>;

	class IImageSource
	{
	public:
		virtual void SetImage(ImageFuture future) = 0;
	};
}
