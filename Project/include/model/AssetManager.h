#ifndef ASSET_MANAGER_H__
#define ASSET_MANAGER_H__
#pragma once

#include "model/Asset.h"
#include "util/Security.h"
#include "fs/AssetDatabase.h"
#include <expected>
#include <ranges>
#include <set>
#include <mutex>
#include <future>

namespace fig::user
{
	class UserManager;
	struct UserProfile;
}

namespace fig::io
{
	struct CharacterData;

	using AsyncResult_Image		= fig::sdl::Surface;
	using AsyncResult_CoverPair	= std::pair<fig::sdl::Surface, fig::sdl::Surface>;
	using AsyncResultVariant = std::variant<AsyncResult_Image, AsyncResult_CoverPair>;

	using AsyncPromise = std::promise<std::expected<AsyncResultVariant, AsyncLoadError>>;
	using AsyncFuture = std::future<std::expected<AsyncResultVariant, AsyncLoadError>>;

	enum class AsyncTask {
		None,
		LoadPortrait,
		LoadCoverImage,
	};

	struct AsyncLoad
	{
		uint64_t id;
		fig::uuid assetId;
		AsyncTask task {};
		AsyncFuture future;
	};

	class AssetManager
	{
		AssetManager() = delete;
	public:
		explicit AssetManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey, int32_t worker_threads = 2);
		virtual ~AssetManager();

		const Asset& CreateEmptyAsset(AssetType type, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent = {}) noexcept;

		const Asset& CreateImageAsset(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateImageAsset(ImageType subtype, DataFormat format, fig::byte_span data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateImageAsset(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept;

		bool DeleteAsset(const fig::uuid& assetID) noexcept;
		uint32_t DeleteAssets(std::span<fig::uuid> assetIDs) noexcept;

		std::optional<AssetRef> FindAsset(const fig::uuid& id) noexcept;
		std::optional<AssetRef> FindAsset(const fig::uuid& id, AssetType assetType) noexcept;
		std::optional<AssetRef> FindAsset(const fig::uuid& parentId, ImageType imageType) noexcept;

		FileError LoadAsset(const Asset& asset) noexcept;
		std::expected<AssetRef, FileError> LoadAsset(const fig::uuid& id) noexcept;
		
		auto GetAssets() noexcept { return _assets | std::views::values; }
		auto GetAssets() const noexcept { return _assets | std::views::values; }
		auto GetAssetsOfType(AssetType assetType) const noexcept { return _assets | std::views::values | std::views::filter([assetType](auto& a) { return a.asset_type == assetType; }); }
		auto GetCharacterAssets() const noexcept { return GetAssetsOfType(AssetType::Character); }
		auto GetScenarioAssets() const noexcept { return GetAssetsOfType(AssetType::Scenario); }
		

		void SaveModified();

		enum class CharacterDataFormat { Default, TavernV2, };
		std::vector<AssetRef> ImportCharactersInDirectory(const fig::path& directory, CharacterDataFormat format = CharacterDataFormat::Default, size_t max_count = 0uz);
		std::expected<AssetRef, FileError> ImportCharacter(const fig::path& filename, CharacterDataFormat format = CharacterDataFormat::Default);
		std::expected<AssetRef, FileError> ImportScenario(const fig::path& filename);
		
		static FileError CreateProfilePicture(const fig::user::UserProfile& profile, fig::path imageFilename);

		[[nodiscard]] AsyncLoad LoadAssetAsync(const fig::uuid& assetId, AsyncTask task, int32_t priority);
		void Cancel(const fig::uuid& assetId);
		void CancelAll();

	private:
		AssetDatabase& GetDatabase() noexcept;
		fig::uuid NewUUID() const noexcept;
		
		std::expected<CharacterData, FileError> LoadCharacterData(fig::path filename, CharacterDataFormat format = CharacterDataFormat::Default);
		size_t LoadAssetIndex() noexcept;
		bool LoadAssetMetaData() noexcept;
		bool LoadDataAssets() noexcept;
		bool UpdateAssetOnDisk(Asset& asset);
		bool UpdateAssetInDatabase(Asset& asset);
		std::expected<AssetRef, FileError> LoadAsset_Internal(Asset& asset) noexcept;
		bool DeleteAsset_Internal(const fig::uuid& assetID) noexcept;
		bool DeleteAssetFile(const fig::uuid& assetID) noexcept;
		std::set<fig::uuid> FindRelatedAssets(const fig::uuid& assetId) noexcept;

	private:
		fig::uuid _profileID;
		fig::path _profilePath;
		fig::auth::AuthKey _profileAuthKey {};
		std::map<fig::uuid, AssetFolder> _folders {};
		std::map<fig::uuid, Asset> _assets {};
		std::unique_ptr<AssetDatabase> _pAssetDB;

		std::mutex _assetsMutex; // Guards _assets

		/* Internal; Mutex already held: */
		Asset& CreateEmptyAsset_Internal(AssetType type, const fig::uuid& parent) noexcept;
		Asset& CreateAsset_Internal(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept;
		Asset& CreateAsset_Internal(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept;
		Asset& CreateImageAsset_Internal(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept;
		Asset& CreateImageAsset_Internal(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept;
		std::expected<AssetRef, FileError> ImportCharacter_Internal(const fig::path& filename, CharacterDataFormat format);
		std::expected<AssetRef, FileError> ImportScenario_Internal(const fig::path& filename);
	private:
		/* Asynchronous loading */
		void __Worker(std::stop_token stop);
		AsyncLoadError __LoadImageTask(const fig::uuid& characterAssetID, ImageType imageType, AsyncResultVariant& outResult) noexcept;
		AsyncLoadError __LoadCoverImageTask(const fig::uuid& characterAssetID, AsyncResultVariant& outResult) noexcept;

		struct PendingRequest {
			uint64_t id {};
			fig::uuid assetId {};
			int32_t priority {};
			AsyncTask task {};

			std::unique_ptr<AsyncPromise> promise;

			bool operator<(const PendingRequest& rhs) const noexcept
			{
				return priority < rhs.priority;
			}
		};

		[[nodiscard]] bool IsAsyncRequestAlive(const PendingRequest& request) const;

		std::priority_queue<PendingRequest> _pending;
		mutable std::mutex _pending_mutex;
		std::condition_variable _pending_cv;
		std::map<fig::uuid, AsyncPromise*> _active_promises;
		mutable std::mutex _active_mutex;
		std::atomic<uint64_t>     _next_id { 0 };
		std::vector<std::jthread> _workers;
	};

}
#endif