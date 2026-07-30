#pragma once

#include "Figment.h"
#include "io/Asset.h"
#include <expected>
#include <functional>

struct sqlite3;
struct sqlite3_stmt;

namespace fig::io
{
	class IndexDatabase
	{
	public:
		IndexDatabase(fig::path filename);
		virtual ~IndexDatabase();

		std::expected<std::map<fig::uuid, Asset>, DatabaseError> FetchAssets() noexcept;
		std::expected<std::map<fig::uuid, AssetFolder>, DatabaseError> FetchFolders() noexcept;

		DatabaseError CreateAsset(const Asset& asset) noexcept; //! @todo: remove?
		DatabaseError UpdateAsset(const Asset& asset) noexcept; //! @todo: remove?
		DatabaseError UpsertAsset(const Asset& asset) noexcept;
		DatabaseError DeleteAsset(const fig::uuid& assetID) noexcept;

		std::expected<int32_t, DatabaseError> UpsertAssets(const fig::ref_vector<Asset>& assets) noexcept;
		std::expected<int32_t, DatabaseError> DeleteAssets(std::span<fig::uuid> assetIDs) noexcept;
		
		DatabaseError CreateFolder(const AssetFolder& folder) noexcept;
		DatabaseError DeleteFolder(const fig::uuid& assetID) noexcept;

		bool IsConnected() const noexcept { return _pDB != nullptr; }

	private:
		bool Connect() noexcept;
		bool Disconnect() noexcept;
		bool CreateDatabaseAndConnect() noexcept;
		void PrepareStatements() noexcept;

		enum class SQL
		{
			CreateAssetsTable,

			FetchAssets,
			CreateAsset,
			UpdateAsset,
			UpsertAsset,
			DeleteAsset,

			FetchFolders,
			CreateFolder,
			DeleteFolder,
		};
		DatabaseError BindAndExecute(SQL statement, std::function<void(sqlite3_stmt*)> fnBind);

	private:
		fig::path _filename;
		sqlite3* _pDB = nullptr;
		std::map<SQL, sqlite3_stmt*> _sqlStatements;
	};
}
