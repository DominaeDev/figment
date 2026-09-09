#pragma once

#include "io/AssetManager.h"

namespace fig::io
{
	using AsyncImageLoadResult = std::shared_ptr<fig::sdl::Surface>;
	using AsyncImageLoadCompleteDelegate = std::function<void(AsyncImageLoadResult)>;
	using AsyncImageLoadErrorDelegate = std::function<void(AsyncLoadError)>;

	class AsyncImageLoad
	{
	public:
		void LoadAsync(const fig::uuid& assetId, AsyncImageLoadCompleteDelegate onComplete, AsyncImageLoadErrorDelegate onError = {});

		void Poll();
		void Cancel();

	private:
		fig::uuid _assetId;
		fig::io::AsyncFuture _future {};
		AsyncImageLoadCompleteDelegate _fnOnComplete {};
		AsyncImageLoadErrorDelegate _fnOnError {};
	};
}