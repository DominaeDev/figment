#pragma once

#include "io/Asset.h"
#include "user/Security.h"
#include "io/IndexDatabase.h"
#include <expected>
#include <ranges>
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
		const Asset& CreateAsset(AssetTypeDefinition type, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateAsset(AssetTypeDefinition type, fig::byte_span data, const fig::uuid& parent = {}) noexcept;

		const Asset& CreateImageAsset(ImageAssetType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateImageAsset(ImageAssetType subtype, DataFormat format, fig::byte_span data, const fig::uuid& parent = {}) noexcept;
		const Asset& CreateImageAsset(ImageAssetType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent = {}) noexcept;

		bool DeleteAsset(fig::uuid assetId) noexcept;
		size_t DeleteAssets(std::span<fig::uuid> assetIds) noexcept;
		bool ReleaseAssetData(const fig::uuid& assetId) noexcept;

		fig::optional_cref<Asset> FindAsset(const fig::uuid& assetId) noexcept;
		fig::optional_cref<Asset> FindAsset(const fig::uuid& assetId, AssetType assetType) noexcept;
		template<asset_subtype_type T>
		fig::optional_cref<Asset> FindAsset(const fig::uuid& assetId, AssetType assetType, T subtype) noexcept
		{
			std::scoped_lock lock { _assetsMutex };
			return FindAsset_NoLock(assetId, make_asset_type(assetType, subtype));
		}
		fig::optional_cref<Asset> FindAssetOfType(AssetTypeDefinition type, const fig::uuid& parentId = {}) noexcept;

		fig::cref_vector<Asset> FindChildrenOf(const fig::uuid& parentId) noexcept;
		bool HasChildren(const fig::uuid& assetId) noexcept;
		std::set<fig::uuid> GetAssociatedAssets(const fig::uuid& assetId) noexcept;

		FileError LoadAsset(const Asset& asset) noexcept;
		fig::expected_cref<Asset, FileError> LoadAsset(const fig::uuid& assetId) noexcept;
		void LoadAssetData(const std::vector<fig::uuid>& assetIds) noexcept;
		void LoadAssetData(const fig::ref_vector<Asset>& assets) noexcept;
		
		auto GetAssets() noexcept { return _assets | std::views::values; }
		auto GetAssets() const noexcept { return _assets | std::views::values; }
		auto GetAssetsOfType(AssetType assetType) const noexcept { 
			return _assets 
				| std::views::filter([assetType](auto&& kvp) { return (kvp.second).type.IsOfType(assetType); }) 
				| std::views::values;
		}
		template <asset_subtype_type T>
		auto GetAssetsOfType(AssetType assetType, T subtype) const noexcept { 
			return _assets 
				| std::views::filter([assetType, subtype](auto&& kvp) { return (kvp.second).type.IsOfType(assetType, subtype); })
				| std::views::values;
		}

		auto GetCharacterAssets() const noexcept { return GetAssetsOfType(AssetType::Character); }
		auto GetScenarioAssets() const noexcept { return GetAssetsOfType(AssetType::Scenario); }

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

		void SaveNow();
		void Shutdown();

		enum class CharacterDataFormat { Default, TavernV2, };
		fig::ref_vector<Asset> ImportCharactersInDirectory(const fig::path& directory, CharacterDataFormat format = CharacterDataFormat::Default, size_t max_count = 0uz);
		fig::expected_ref<Asset, FileError> ImportCharacter(const fig::path& filename, CharacterDataFormat format = CharacterDataFormat::Default);
		fig::expected_ref<Asset, FileError> ImportScenario(const fig::path& filename);
		
		static FileError CreateProfilePicture(const fig::user::UserProfile& profile, fig::path imageFilename);

		[[nodiscard]] AsyncLoad LoadAssetAsync(const fig::uuid& assetId, AsyncTask task, int32_t priority);
		void Cancel(const fig::uuid& assetId);
		void CancelAll();

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
		fig::optional_cref<Asset> FindAsset_NoLock(const fig::uuid& assetId) noexcept;
		fig::optional_cref<Asset> FindAsset_NoLock(const fig::uuid& assetId, AssetType assetType) noexcept;
		fig::optional_cref<Asset> FindAsset_NoLock(const fig::uuid& assetId, AssetTypeDefinition assetType) noexcept;
		int32_t GetAssetDepth_NoLock(const fig::uuid& id, std::unordered_map<fig::uuid, int32_t>& depthCache, int32_t depth = 0) const noexcept;

		size_t DeleteAssets_NoLock(std::span<fig::uuid> assetIds) noexcept;
		size_t DeleteAssetFiles_NoLock(std::span<fig::uuid> assetIds) noexcept;
		std::unordered_set<fig::uuid> FindRelatedAssets_NoLock(const fig::uuid& assetId) noexcept;

	private:
		fig::uuid _profileID;
		fig::path _profilePath;
		fig::auth::AuthKey _profileAuthKey {};
		std::map<fig::uuid, AssetFolder> _folders {};
		std::map<fig::uuid, Asset> _assets {};
		std::unique_ptr<IndexDatabase> _pAssetDB;

		std::mutex _assetsMutex; // Guards _assets

		void ModifyAsset_Void(const fig::uuid& assetId, std::function<void(Asset&)> fn);
		void ModifyAsset_Void(const Asset& asset, std::function<void(Asset&)> fn);
		bool ModifyAsset_Bool(const fig::uuid& assetId, std::function<bool(Asset&)> fn);
		bool ModifyAsset_Bool(const Asset& asset, std::function<bool(Asset&)> fn);


		/* Internal; Mutex is locked */
		Asset& CreateEmptyAsset_NoLock(AssetTypeDefinition type, const fig::uuid& parent) noexcept;
		Asset& CreateAsset_NoLock(AssetTypeDefinition type, fig::bytes&& data, const fig::uuid& parent) noexcept;
		Asset& CreateAsset_NoLock(AssetTypeDefinition type, fig::byte_span data, const fig::uuid& parent) noexcept;
		Asset& CreateImageAsset_NoLock(ImageAssetType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept;
		Asset& CreateImageAsset_NoLock(ImageAssetType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept;
		fig::expected_ref<Asset, FileError> ImportCharacter_NoLock(const fig::path& filename, CharacterDataFormat format);
		fig::expected_ref<Asset, FileError> ImportScenario_NoLock(const fig::path& filename);

	private:
		bool SaveModifiedAssets();

		/* Asynchronous loading */
		void __Worker(std::stop_token stop);
		AsyncLoadError __LoadImageTask(const fig::uuid& characterAssetID, ImageAssetType imageType, AsyncResultVariant& outResult) noexcept;
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

		/* Autosave*/
		void __Autosave(std::stop_token stopToken, std::chrono::seconds interval);

	private:
		std::priority_queue<PendingRequest> _pending;
		mutable std::mutex _pending_mutex;
		std::condition_variable _pending_cv;
		std::map<fig::uuid, AsyncPromise*> _active_promises;
		mutable std::mutex _active_mutex;
		std::atomic<uint64_t> _next_id { 0 };
		std::vector<std::jthread> _workers;
		std::jthread _autosave_worker {};
		std::condition_variable_any _autosave_cv {};
	};

}
