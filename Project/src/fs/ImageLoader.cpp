#include <pch.h>
#include "fs/ImageLoader.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "Constants.h"
#include <cassert>

using namespace fig::io::data;
using namespace fig::gui;
using namespace fig::gui::util;

namespace fig::io
{
	ImageLoader::ImageLoader(int num_threads)
	{
		_workers.reserve(num_threads);
		for (int i = 0; i < num_threads; ++i)
			_workers.emplace_back([this](std::stop_token st) { WorkerThread(st); });
	}

	void ImageLoader::Release()
	{
		_images.clear();
		_textures.clear();
		_coverImages.clear();
	}

	static bool CreateSurface(const Asset& cover, fig::sdl::Surface& out_surface)
	{
		// Create SDL surface
		int32_t width = cover.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(Constants::GUI::HomeScreen::CardWidth);
		int32_t height = cover.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(Constants::GUI::HomeScreen::CardHeight);
		int32_t depth = cover.GetMeta<uint8_t>(MetaTag::ImageFormatDepth).value_or(4);

		try
		{
			SurfacePtr pSurface = SDL_CreateSurface(width, height, depth == 3 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA8888);
			if (!pSurface)
				return false;

			if (SDL_LockSurface(pSurface))
			{
				assert(pSurface->pitch * pSurface->h == cover.data.size());
				std::memcpy(pSurface->pixels, cover.data.data(), cover.data.size());
				SDL_UnlockSurface(pSurface);

				out_surface.reset(pSurface);
				return true;
			}		
		}
		catch (...)
		{
		}
		return false;
	}

	bool ImageLoader::LoadCoverImage(const fig::uuid& characterAssetID)
	{
		auto& assetMngr = Global::GetUserManager().GetProfileAssets();

		if (auto findCover = assetMngr.FindAsset(characterAssetID, ImageType::CoverImage))
		{
			Asset& cover = findCover.value();
			if (_images.contains(cover.id))
				return true; // Already loaded

			if (assetMngr.LoadAsset(cover) == FileError::NoError)
			{
				fig::sdl::Surface surface;
				if (CreateSurface(cover, surface))
				{
					_images[cover.id] = std::move(surface);
					_coverImages[characterAssetID] = cover.id;
					return true;
				}
			}
		}

		// Create cover from portrait
		if (auto findPortrait = assetMngr.FindAsset(characterAssetID, ImageType::LargePortrait))
		{
			Asset& portraitAsset = findPortrait.value();
			if (assetMngr.LoadAsset(portraitAsset) == FileError::NoError)
			{
				if (auto portraitImage = LoadImageFromMemory(portraitAsset.data))
				{
					auto coverImage = ScaleSurface(portraitImage, Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight, ImageFit::Portrait);

					// Round corners
					MaskCorners(coverImage, CornerStyle::Card);

					// Save cover asset (bitmap)
					auto& coverAsset = assetMngr.CreateImageAsset(ImageType::CoverImage, coverImage, characterAssetID);
					coverAsset.SetMeta(MetaTag::Version, uint8_t { 1 });
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);

					_images[coverAsset.id] = std::move(coverImage);
					_coverImages[characterAssetID] = coverAsset.id;
					return true;
				}
			}
		}
		return false;
	}

	fig::uuid ImageLoader::GetCoverImageID(const fig::uuid& characterAssetID) noexcept
	{
		auto itFind = _coverImages.find(characterAssetID);
		if (itFind != _coverImages.cend())
			return itFind->second;
		return {};
	}

	TexturePtr ImageLoader::GetTexture(RendererPtr pRenderer, const fig::uuid& assetID)
	{
		auto itTexture = _textures.find(assetID);
		if (itTexture != _textures.end())
			return itTexture->second.get();

		auto itFind = _images.find(assetID);
		if (itFind != _images.end())
		{
			Surface* pSurface = itFind->second.get();
			auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
			if (!pTexture)
				return nullptr;

			fig::sdl::Texture texture;
			texture.reset(pTexture);
			_textures[assetID] = std::move(texture);
			return pTexture;
		}
		return nullptr; // Not found
	}

	void ImageLoader::WorkerThread(std::stop_token stop)
	{
		while (!stop.stop_requested())
		{
			PendingRequest request;

			// Block until work arrives
			{
				std::unique_lock lock(_pending_mutex);
				_pending_cv.wait(lock, [&] {
					return !_pending.empty() || stop.stop_requested();
				});
				if (stop.stop_requested()) 
					break; // Stop

				request = std::move(const_cast<PendingRequest&>(_pending.top()));
				_pending.pop();
			}

			// If promise is already gone (cancelled via active_promises),
			//    the unique_ptr will be non-null but the promise may have been
			//    fulfilled already. Check via _active_promises instead.
			if (!IsPromiseStillActive(request.assetId, request.promise.get()))
			{
				// Promise was cancelled — already fulfilled with nullptr.
				// Just let request (and its unique_ptr) drop.
				continue;
			}

			// Disk I/O.
			SDL_Surface* pSurface = IMG_Load(request.path.c_str());
			if (!pSurface)
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					"ImageLoader: IMG_Load(\"%s\") failed: %s",
					request.path.c_str(), SDL_GetError());
			}

			// Check again in case of cancellation during I/O.
			if (!IsPromiseStillActive(request.assetId, request.promise.get()))
			{
				if (pSurface)
					SDL_DestroySurface(pSurface);
				continue;
			}

			// Fulfil the promise (surface may be nullptr on load failure).
			//    Remove from active_promises_ first so Cancel() won't double-set.
			{
				std::scoped_lock<std::mutex> lock(_active_mutex);
				_active_promises.erase(request.assetId);
			}

			fig::sdl::Surface surface;
			surface.reset(pSurface);
			request.promise->set_value(std::move(surface));
		}
	}

	[[nodiscard]] ImageLoader::EnqueuedLoad ImageLoader::Enqueue(IImageSource* card, const fig::uuid& assetId, int priority)
	{
		const uint64_t id = _next_id.fetch_add(1, std::memory_order_relaxed);

		// Cancel the previous request for this card (if any) by fulfilling
		// its promise with nullptr — clean signal, no exception.
		{
			std::scoped_lock lock(_active_mutex);
			if (auto it = _active_promises.find(assetId); it != _active_promises.end())
			{
				it->second->set_value({});
				_active_promises.erase(it);
			}
		}

		// Create the promise. Share a raw pointer in active_promises_ so we
		// can cancel it on a subsequent Enqueue() or Cancel() call.
		auto promise = std::make_unique<ImagePromise>();
		auto future = promise->get_future();
		auto* promise_p = promise.get();

		{
			std::scoped_lock lock(_active_mutex);
			_active_promises[assetId] = promise_p;
		}
		{
			std::scoped_lock lock(_pending_mutex);
			_pending.push(PendingRequest {
				.id = id,
				.assetId = assetId,
				.priority = priority,
				.promise = std::move(promise),
			});
		}
		_pending_cv.notify_one();

		return EnqueuedLoad {
			.id = id,
			.future = std::move(future),
		};
	}

	// Cancel any pending load for `card`, fulfilling its future with nullptr.
	void ImageLoader::Cancel(const fig::uuid& assetId)
	{
		std::scoped_lock lock(_active_mutex);
		if (auto it = _active_promises.find(assetId); it != _active_promises.end())
		{
			it->second->set_value({});
			_active_promises.erase(it);
		}
	}

	void ImageLoader::CancelAll()
	{
		{
			// Clear pending queue
			std::scoped_lock lock(_pending_mutex);
			while (not _pending.empty())
				_pending.pop();
		}

		{
			// Cancel active promises
			std::scoped_lock lock(_active_mutex);
			for (auto& promise : _active_promises)
				promise.second->set_value({});
			_active_promises.clear();
		}
	}

	bool ImageLoader::IsPromiseStillActive(const fig::uuid& assetId, const ImagePromise* p) const
	{
		std::scoped_lock lock(_active_mutex);
		auto it = _active_promises.find(assetId);
		return it != _active_promises.end() && it->second == p;
	}

}