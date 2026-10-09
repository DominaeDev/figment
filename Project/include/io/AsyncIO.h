#pragma once

#include "io/Asset.h"
#include <future>

namespace fig::user
{
	class UserManager;
	struct UserProfile;
}

namespace fig::data
{
	class Character;
}

namespace fig::io
{
	using AsyncResult_Image = fig::sdl::Surface;
	using AsyncResult_CoverPair = std::pair<fig::sdl::Surface, fig::sdl::Surface>;
	using AsyncResultVariant = std::variant<AsyncResult_Image, AsyncResult_CoverPair>;
	using AsyncResult = std::shared_ptr<AsyncResultVariant>;

	using AsyncPromise = std::promise<std::expected<AsyncResult, AsyncLoadError>>;
	using AsyncFuture = std::future<std::expected<AsyncResult, AsyncLoadError>>;

	template <typename T>
	std::expected<std::shared_ptr<T>, AsyncLoadError> GetAsyncResult(AsyncFuture& future)
	{
		if (future.valid() and future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			if (auto result = future.get(); result.has_value())
			{
				if (auto pValue = std::get_if<T>((*result).get()))
					return std::shared_ptr<T>(*result, pValue);
			}
			else
				return std::unexpected(result.error());
		}
		return std::unexpected(AsyncLoadError::NoError); // No result yet
	}
}