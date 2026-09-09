#include <pch.h>
#include "io/AsyncImageLoad.h"

namespace fig::io
{
	void AsyncImageLoad::LoadAsync(const fig::uuid& assetId, AsyncImageLoadCompleteDelegate onComplete, AsyncImageLoadErrorDelegate onError)
	{
		_assetId = assetId;
		_fnOnComplete = onComplete;
		_fnOnError = onError;

		if (auto request = Global::GetUserContent().GetAssets().LoadAssetAsync(assetId, AsyncTask::LoadImage, 0); request.future.valid())
			_future = std::move(request.future);
		else if (_fnOnError)
			_fnOnError(AsyncLoadError::LoadError);
	}

	void AsyncImageLoad::Poll()
	{
		if (not _future.valid() or _assetId.empty())
			return;

		if (auto try_surface = GetAsyncResult<fig::sdl::Surface>(_future))
		{
			_assetId = {};
			if (_fnOnComplete)
				_fnOnComplete(*try_surface);
		}
		else if (try_surface.error() != AsyncLoadError::NoError)
		{
			_assetId = {};
			if (_fnOnError)
				_fnOnError(try_surface.error());
		}
	}

	void AsyncImageLoad::Cancel()
	{
		if (not _future.valid() or _assetId.empty())
			return;

		Global::GetUserContent().GetAssets().CancelAsync(_assetId);
	}
}