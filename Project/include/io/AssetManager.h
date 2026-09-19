#pragma once

#include "io/Asset.h"
#include "user/Security.h"
#include "io/IndexDatabase.h"
#include <mutex>
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
	using AsyncResult_Image		= fig::sdl::Surface;
	using AsyncResult_CoverPair	= std::pair<fig::sdl::Surface, fig::sdl::Surface>;
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

	enum class AsyncTask {
		None,
		LoadImage,
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

	using ModifyAssetsDelegate = std::function<void(const fig::ref_vector<Asset>&)>;

	class AssetManager
	{
		AssetManager() = delete;
	public:
		explicit AssetManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey, int32_t worker_threads = 2);
		virtual ~AssetManager();

		const Asset& CreateAsset(AssetType type, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent = {}, bool bChecksum = false) noexcept;
		const Asset& CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent = {}, bool bChecksum = false) noexcept;
		const Asset& CreateAsset(AssetTypeDefinition type, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetTypeDefinition type, fig::bytes&& data, const fig::uuid& parent = {}, bool bChecksum = false) noexcept;
		const Asset& CreateAsset(AssetTypeDefinition type, fig::byte_span data, const fig::uuid& parent = {}, bool bChecksum = false) noexcept;
		const Asset& CreateImageAsset(ImageAssetType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent = {}) noexcept;

		bool UpdateAsset(const fig::uuid& assetId, fig::bytes&& data, bool bChecksum = false) noexcept;
		bool UpdateAsset(const fig::uuid& assetId, fig::byte_span data, bool bChecksum = false) noexcept;

		bool DeleteAsset(fig::uuid assetId) noexcept;
		size_t DeleteAssets(std::span<fig::uuid> assetIds) noexcept;
		bool ReleaseAssetData(const fig::uuid& assetId) noexcept;

		fig::cref_vector<Asset> GetAllAssets() const noexcept;
		fig::optional_cref<Asset> FindAsset(const fig::uuid& assetId) const noexcept;
		fig::optional_cref<Asset> FindAsset(const fig::uuid& assetId, AssetType assetType) const noexcept;
		template<asset_subtype_type T>
		fig::optional_cref<Asset> FindAsset(const fig::uuid& assetId, AssetType assetType, T subtype) const noexcept
		{
			std::scoped_lock lock { _assetsMutex };
			return FindAsset_NoLock(assetId, make_asset_type(assetType, subtype));
		}
		fig::optional_cref<Asset> FindAssetOfType(AssetTypeDefinition type, const fig::uuid& parentId = {}) const noexcept;
		fig::cref_vector<Asset> FindAssetsOfType(AssetTypeDefinition type, const fig::uuid& parentId = {}) const noexcept;

		fig::cref_vector<Asset> FindChildrenOf(const fig::uuid& parentId) const noexcept;
		bool HasChildren(const fig::uuid& assetId) const noexcept;
		std::set<fig::uuid> FindAssociatedAssets(const fig::uuid& assetId) const noexcept;

		FileError LoadAsset(const Asset& asset) noexcept;
		fig::expected_cref<Asset, FileError> LoadAsset(const fig::uuid& assetId) noexcept;
		void LoadAssetData(const std::vector<fig::uuid>& assetIds) noexcept;
		void LoadAssetData(const fig::ref_vector<Asset>& assets) noexcept;
		
		template <typename Fn>
		decltype(auto) ModifyAsset(const fig::uuid& assetId, Fn fn)
		{
			if constexpr (std::is_invocable_r_v<bool, Fn, Asset&>)
				return ModifyAsset_Bool(assetId, fn);
			else
				return ModifyAsset_Void(assetId, fn);
		}

		template <typename Fn>
		decltype(auto) ModifyAsset(const Asset& asset, Fn fn)
		{
			if constexpr (std::is_invocable_r_v<bool, Fn, Asset&>)
				return ModifyAsset_Bool(asset, fn);
			else
				return ModifyAsset_Void(asset, fn);
		}

		void ModifyAssets(const std::vector<fig::uuid> assetIds, ModifyAssetsDelegate fnDelegate);

		void SaveNow();
		void Shutdown();

		enum class CharacterDataFormat { Default, TavernV2, };
		fig::ref_vector<Asset> ImportCharactersInDirectory(const fig::path& directory, CharacterDataFormat format = CharacterDataFormat::Default, size_t max_count = 0uz);
		fig::expected_ref<Asset, FileError> ImportCharacter(const fig::path& filename, CharacterDataFormat format = CharacterDataFormat::Default);
		fig::expected_ref<Asset, FileError> ImportScenario(const fig::path& filename);
		
		static FileError CreateProfilePicture(const fig::user::UserProfile& profile, fig::path imageFilename);

		[[nodiscard]] AsyncLoad LoadAssetAsync(const fig::uuid& assetId, AsyncTask task, int32_t priority);
		void CancelAsync(const fig::uuid& assetId);
		void CancelAllAsync();

	private:
		IndexDatabase& GetDatabase() noexcept;
		fig::uuid GenerateUUID() const noexcept;
		
		std::expected<fig::data::Character, FileError> LoadCharacterData(fig::path filename, CharacterDataFormat format = CharacterDataFormat::Default);
		size_t LoadIndexDatabase() noexcept;

		bool LoadMetaData(AssetType assetType) noexcept;
		bool LoadAssetData() noexcept;

		bool WriteAssetToDisk(Asset& asset);

		/* Internal */
		fig::expected_ref<Asset, FileError> LoadAsset_NoLock(Asset& asset) noexcept;
		fig::expected_ref<Asset, FileError> LoadAssetMeta_NoLock(Asset& asset) noexcept;
		fig::optional_cref<Asset> FindAsset_NoLock(const fig::uuid& assetId) const noexcept;
		fig::optional_cref<Asset> FindAsset_NoLock(const fig::uuid& assetId, AssetType assetType) const noexcept;
		fig::optional_cref<Asset> FindAsset_NoLock(const fig::uuid& assetId, AssetTypeDefinition assetType) const noexcept;
		int32_t GetAssetDepth_NoLock(const fig::uuid& id, std::unordered_map<fig::uuid, int32_t>& depthCache, int32_t depth = 0) const noexcept;
		std::unordered_set<fig::uuid> FindRelatedAssets_NoLock(const fig::uuid& assetId) const noexcept;

		size_t DeleteAssets_NoLock(std::span<fig::uuid> assetIds) noexcept;
		size_t DeleteAssetFiles_NoLock(std::span<fig::uuid> assetIds) noexcept;

		void ModifyAsset_Void(const fig::uuid& assetId, std::function<void(Asset&)> fn);
		void ModifyAsset_Void(const Asset& asset, std::function<void(Asset&)> fn);
		bool ModifyAsset_Bool(const fig::uuid& assetId, std::function<bool(Asset&)> fn);
		bool ModifyAsset_Bool(const Asset& asset, std::function<bool(Asset&)> fn);

		/* Internal; Only called when mutex is locked */
		Asset& CreateAsset_NoLock(AssetTypeDefinition type, const fig::uuid& parent) noexcept;
		Asset& CreateAsset_NoLock(AssetTypeDefinition type, fig::bytes&& data, const fig::uuid& parent, bool bChecksum) noexcept;
		Asset& CreateAsset_NoLock(AssetTypeDefinition type, fig::byte_span data, const fig::uuid& parent, bool bChecksum) noexcept;
		Asset& CreateImageAsset_NoLock(ImageAssetType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept;
		bool UpdateAsset_NoLock(const fig::uuid& assetId, fig::bytes&& data, bool bChecksum) noexcept;
		bool UpdateAsset_NoLock(const fig::uuid& assetId, fig::byte_span data, bool bChecksum) noexcept;

		fig::expected_ref<Asset, FileError> ImportCharacter_NoLock(const fig::path& filename, CharacterDataFormat format);
		fig::expected_ref<Asset, FileError> ImportScenario_NoLock(const fig::path& filename);

	private:
		bool SaveModifiedAssets();

		/* Asynchronous loading */
		void __Worker(std::stop_token stop);
		AsyncLoadError __LoadImageTask(const fig::uuid& assetId, AsyncResult& outResult) noexcept;
		AsyncLoadError __LoadCharacterImageTask(const fig::uuid& characterAssetID, ImageAssetType imageType, AsyncResult& outResult) noexcept;
		AsyncLoadError __LoadCoverImageTask(const fig::uuid& characterAssetID, AsyncResult& outResult) noexcept;
		void __YieldAsyncResult(const fig::uuid& assetId, std::expected<AsyncResult, AsyncLoadError> result);

		struct PendingRequest {
			uint64_t id {};
			fig::uuid assetId {};
			int32_t priority {};
			AsyncTask task {};

			bool operator<(const PendingRequest& rhs) const noexcept
			{
				return priority < rhs.priority;
			}
		};

		[[nodiscard]] bool IsAsyncRequestAlive(const PendingRequest& request) const;

		/* Autosave*/
		void __Autosave(std::stop_token stopToken, std::chrono::seconds interval);

	private:
		fig::uuid _profileID;
		fig::path _profilePath;
		fig::auth::AuthKey _profileAuthKey {};
		std::map<fig::uuid, AssetFolder> _folders {};
		std::map<fig::uuid, Asset> _assets {};
		std::unique_ptr<IndexDatabase> _pAssetDB;

		mutable std::mutex _assetsMutex; // Guards _assets

		// Task queue
		std::priority_queue<PendingRequest> _pending;
		mutable std::mutex _pending_mutex;
		std::condition_variable _pending_cv;
		std::atomic<uint64_t> _next_id { 0 };
		std::vector<std::jthread> _workers;

		std::unordered_map<fig::uuid, std::vector<AsyncPromise>> _active_promises;
		mutable std::mutex _active_mutex;

		// Auto save
		std::jthread _autosave_worker {};
		std::condition_variable_any _autosave_cv {};
	};

}
