#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include "fs/ImageSource.h"
#include <map>
#include <mutex>

namespace fig::io
{
	class ImageLoader
	{
	public:
		explicit ImageLoader(int num_threads = 2);
		void Release();

		bool LoadCoverImage(const fig::uuid& characterAssetID);
		fig::uuid GetCoverImageID(const fig::uuid& characterAssetID) noexcept;
		fig::gui::TexturePtr GetTexture(fig::gui::RendererPtr pRenderer, const fig::uuid& assetID);

		struct EnqueuedLoad
		{
			uint64_t id;
			ImageFuture future;
		};
		[[nodiscard]] EnqueuedLoad Enqueue(IImageSource* source, const fig::uuid& assetId, int priority);
		void Cancel(const fig::uuid& assetId);
		void CancelAll();

	private:
		void WorkerThread(std::stop_token stop);
		[[nodiscard]] bool IsPromiseStillActive(const fig::uuid& assetId, const ImagePromise* p) const;

		struct PendingRequest {
			uint64_t id;
			fig::uuid assetId;
			int32_t priority;

			std::unique_ptr<ImagePromise> promise;

			bool operator<(const PendingRequest& rhs) const noexcept
			{
				return priority < rhs.priority;
			}
		};

		std::priority_queue<PendingRequest> _pending;
		mutable std::mutex                  _pending_mutex;
		std::condition_variable             _pending_cv;

		// Raw (non-owning) pointers into the promises still sitting in the queue.
		// Used to detect cancellation and to fulfil on Cancel() / re-Enqueue().
		std::unordered_map<fig::uuid, ImagePromise*> _active_promises;
		mutable std::mutex _active_mutex;

		std::atomic<uint64_t>     _next_id { 0 };
		std::vector<std::jthread> _workers;

	private:
		std::map<fig::uuid, fig::uuid> _coverImages;
		std::map<fig::uuid, fig::sdl::Surface> _images;
		std::map<fig::uuid, fig::sdl::Texture> _textures;
	};
}