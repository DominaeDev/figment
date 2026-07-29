#include <pch.h>
#include "io/IndexDatabase.h"
#include "io/FileUtility.h"
#include "io/SQLTransaction.h"
#include <sqlite3.h>

using namespace fig::auth;

static constexpr fig::const_string SQL_Pragmas = 
	"PRAGMA journal_mode=WAL;"
	"PRAGMA synchronous=NORMAL;"
	"PRAGMA foreign_keys=ON;";

static constexpr fig::const_string SQL_CreateTables =
	"CREATE TABLE IF NOT EXISTS Assets ("
	"	id          TEXT     PRIMARY KEY NOT NULL,"
	"	parent      TEXT,"
	"   folder      TEXT,"
	"	type        INTEGER  NOT NULL DEFAULT (0),"
	"	metadata    TEXT,"
	"	createdAt   INTEGER  DEFAULT (CURRENT_TIMESTAMP) NOT NULL,"
	"	updatedAt   INTEGER  NOT NULL DEFAULT (CURRENT_TIMESTAMP),"
	"	FOREIGN KEY (parent) REFERENCES Assets (id) ON DELETE CASCADE ON UPDATE CASCADE,"
	"   FOREIGN KEY (folder) REFERENCES Folders (id) ON DELETE SET NULL ON UPDATE CASCADE"
	");"

	"CREATE TABLE IF NOT EXISTS Folders ("
	"	id       TEXT PRIMARY KEY NOT NULL,"
	"	parent   TEXT,"
	"	category INTEGER NOT NULL DEFAULT (0),"
	"	name     TEXT NOT NULL,"
	"	metadata TEXT,"
	"	FOREIGN KEY (parent) REFERENCES Folders(id) ON DELETE SET NULL ON UPDATE CASCADE"
	");"

	"CREATE INDEX Assets_parent_idx ON Assets (parent);"
	"CREATE INDEX Assets_folder_idx ON Assets (folder);"
	"CREATE INDEX Folders_parent_idx ON Folders (parent);"
	"\0";

static constexpr fig::const_string SQL_FetchAsset =
	"SELECT id, parent, folder, type, metadata, createdAt, updatedAt FROM Assets;";

static constexpr fig::const_string SQL_InsertAsset =
	"INSERT INTO Assets (id, parent, folder, type, metadata, createdAt, updatedAt) VALUES (?, ?, ?, ?, ?, ?, ?);";

static constexpr fig::const_string SQL_UpdateAsset =
	"UPDATE Assets SET parent = ?, folder = ?, type = ?, metadata = ?, updatedAt = ? WHERE id = ?;";

static constexpr fig::const_string SQL_DeleteAsset =
	"DELETE FROM Assets WHERE id = ?;";

static constexpr fig::const_string SQL_UpsertAsset =
	"INSERT INTO Assets (id, parent, folder, type, metadata, createdAt, updatedAt)"
	"VALUES (?, ?, ?, ?, ?, ?, ?)"
	"ON CONFLICT (id) DO UPDATE SET"
	"	parent = excluded.parent,"
	"	folder = excluded.folder,"
	"	type = excluded.type,"
	"	metadata = excluded.metadata,"
	"	updatedAt = excluded.updatedAt;";

namespace fig::io
{
	IndexDatabase::IndexDatabase(fig::path filename) :
		_filename { filename }
	{
		// Create database file
		if (not std::filesystem::exists(filename))
			CreateDatabaseAndConnect();
		else
			Connect();
	}

	IndexDatabase::~IndexDatabase()
	{
		Disconnect();
	}

	bool IndexDatabase::Connect() noexcept
	{
		if (_pDB)
			return true; // Already connected

		if (_filename.empty())
			return false;
		
		int rc;
		if (rc = sqlite3_open(_filename.u8string().c_str(), &_pDB); rc != SQLITE_OK)
		{
			sqlite3_close(_pDB);
			_pDB = nullptr;
			return false;
		}

		rc = sqlite3_exec(_pDB, toCStr(SQL_Pragmas), nullptr, nullptr, nullptr);
		PrepareStatements();
		return true;
	}

	bool IndexDatabase::CreateDatabaseAndConnect() noexcept
	{
		if (_pDB)
			return SQLITE_OK; // Already exists

		// Delete existing
		if (std::filesystem::exists(_filename))
			std::filesystem::remove(_filename);

		// Create empty file
		std::ofstream fs(_filename.u8string().c_str());
		fs.close();

		int rc = sqlite3_open(_filename.u8string().c_str(), &_pDB);
		if (rc != SQLITE_OK)
		{
			sqlite3_close(_pDB);
			_pDB = nullptr;
			return rc;
		}

		// Create tables
		rc = sqlite3_exec(_pDB, toCStr(SQL_CreateTables), nullptr, nullptr, nullptr);
		if (rc != SQLITE_OK)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return false;
		}

		rc = sqlite3_exec(_pDB, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

		PrepareStatements();
		return true;
	}

	bool IndexDatabase::Disconnect() noexcept
	{
		if (_pDB)
		{
			for (auto& stmt : _sqlStatements | std::views::values)
				sqlite3_finalize(stmt);
			_sqlStatements.clear();
			
			sqlite3_close_v2(_pDB);
			_pDB = nullptr;
			return true;
		}
		return false;
	}

	void IndexDatabase::PrepareStatements() noexcept
	{
		auto Prepare = [this](SQL op, fig::string_view sql) {
			int rc = sqlite3_prepare_v2(_pDB, sql.data(), static_cast<int32_t>(sql.length()), &_sqlStatements[op], nullptr);
			assert(rc == SQLITE_OK);
		};

		// Prepare statements
		Prepare(SQL::FetchAssets, SQL_FetchAsset);
		Prepare(SQL::CreateAsset, SQL_InsertAsset);
		Prepare(SQL::UpdateAsset, SQL_UpdateAsset);
		Prepare(SQL::UpsertAsset, SQL_UpsertAsset);
		Prepare(SQL::DeleteAsset, SQL_DeleteAsset);

		Prepare(SQL::FetchFolders, "SELECT id, parent, category, name, metadata FROM Folders;");
		Prepare(SQL::CreateFolder, "INSERT INTO Folders (id, parent, category, name, metadata) VALUES (?, ?, ?, ?, ?);");
		Prepare(SQL::DeleteFolder, "DELETE FROM Folders WHERE id = ?;");
	}

	DatabaseError IndexDatabase::BindAndExecute(SQL statement, std::function<void(sqlite3_stmt*)> fnBind)
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[statement];

		fnBind(stmt);

		int rc = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return DatabaseError::SQLError;
		}

		if (int nUpdates = sqlite3_changes(_pDB); !nUpdates)
		{
			LogLn("SQLite Error: No changes");
			return DatabaseError::ZeroChanges;
		}
		return DatabaseError::NoError;
	}

	std::expected<std::map<fig::uuid, Asset>, DatabaseError> IndexDatabase::FetchAssets() noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		std::map<fig::uuid, Asset> result;

		int rc;
		auto stmt = _sqlStatements[SQL::FetchAssets];
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			const char* pAssetID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			const char* pParentID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			const char* pFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			int32_t type = sqlite3_column_int(stmt, 3);
			const char* pMetaData = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

			int64_t createdAt = sqlite3_column_int64(stmt, 5);
			int64_t updatedAt = sqlite3_column_int64(stmt, 6);

			fig::uuid assetID	= fig::uuid::from_str(pAssetID ? pAssetID : "");
			fig::uuid parentID	= fig::uuid::from_str(pParentID ? pParentID : "");
			fig::uuid folderID	= fig::uuid::from_str(pFolder ? pFolder : "");

			Asset asset;
			asset.id = assetID;
			asset.parent_id = parentID;
			asset.folder_id = folderID;
			asset.type = AssetTypeDefinition::FromRaw(static_cast<uint32_t>(type));
			asset.SetMeta(MetaTag::CreatedAt, fig::timestamp(createdAt));
			asset.SetMeta(MetaTag::UpdatedAt, fig::timestamp(updatedAt));
			if (pMetaData)
				asset.SetUserSettingsJson(fig::string { pMetaData });

			asset.sync_state.file_sync = AssetSyncState::Status::Indeterminate;
			asset.sync_state.db_sync = AssetSyncState::Status::Synchronized;
			if (!asset.id.empty())
				result[asset.id] = std::move(asset);
		}

		sqlite3_reset(stmt);

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return std::unexpected(DatabaseError::SQLError);
		}

		return result;
	}

	std::expected<std::map<fig::uuid, AssetFolder>, DatabaseError> IndexDatabase::FetchFolders() noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		std::map<fig::uuid, AssetFolder> result;

		int rc;
		auto stmt = _sqlStatements[SQL::FetchFolders];
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			const char* pFolderID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			const char* pParentID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			const char* pCategory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			const char* pName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
			const char* pMetaData = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

			fig::uuid folderID	= fig::uuid::from_str(pFolderID ? pFolderID : "");
			fig::uuid parentID	= fig::uuid::from_str(pParentID ? pParentID : "");

			AssetFolder folder;
			folder.id = folderID;
			folder.parent_id = parentID;
			if (pName)
				folder.name = fig::string { pName };
			if (pCategory)
				folder.category = FolderCategoryFromString(pCategory);
			if (pMetaData)
				folder.settings = fig::string { pMetaData };
			if (!folder.id.empty())
				result[folder.id] = std::move(folder);
		}

		sqlite3_reset(stmt);

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return std::unexpected(DatabaseError::SQLError);
		}

		return result;
	}

	DatabaseError IndexDatabase::CreateAsset(const Asset& asset) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::CreateAsset, [&asset](sqlite3_stmt* stmt) {
			/* id */
			sqlite3_bind_text(stmt, 1, asset.id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			/* parent */
			if (not asset.parent_id.empty())
				sqlite3_bind_text(stmt, 2, asset.parent_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 2, nullptr, -1, SQLITE_STATIC);
			/* folder */
			if (not asset.folder_id.empty())
				sqlite3_bind_text(stmt, 3, asset.folder_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 3, nullptr, -1, SQLITE_STATIC);
			/* type */
			sqlite3_bind_int(stmt, 4, static_cast<int32_t>(asset.type));
			/* metadata */
			if (auto& metadata = asset.GetUserSettingsJson(); not metadata.empty())
				sqlite3_bind_text(stmt, 5, metadata.c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 5, nullptr, -1, SQLITE_STATIC);
			/* createdAt */
			sqlite3_bind_int64(stmt, 6, static_cast<int64_t>(asset.GetCreatedAt()));
			/* updatedAt */
			sqlite3_bind_int64(stmt, 7, static_cast<int64_t>(asset.GetUpdatedAt()));
		});
	}

	DatabaseError IndexDatabase::UpdateAsset(const Asset& asset) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::UpsertAsset, [&asset](sqlite3_stmt* stmt) {
			/* parent */
			if (not asset.parent_id.empty())
				sqlite3_bind_text(stmt, 1, asset.parent_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 1, nullptr, -1, SQLITE_STATIC);
			/* folder */
			if (not asset.folder_id.empty())
				sqlite3_bind_text(stmt, 2, asset.folder_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 2, nullptr, -1, SQLITE_STATIC);
			/* type */
			sqlite3_bind_int(stmt, 3, static_cast<int32_t>(asset.type));
			/* metadata */
			if (auto& metadata = asset.GetUserSettingsJson(); not metadata.empty())
				sqlite3_bind_text(stmt, 4, metadata.c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 4, nullptr, -1, SQLITE_STATIC);
			/* updatedAt */
			sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(asset.GetUpdatedAt()));
			/* id */
			sqlite3_bind_text(stmt, 6, asset.id.to_str().c_str(), -1, SQLITE_TRANSIENT);
		});

		return DatabaseError::NoError;
	}


	DatabaseError IndexDatabase::UpsertAsset(const Asset& asset) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::UpsertAsset, [&asset](sqlite3_stmt* stmt) {
			/* id */
			sqlite3_bind_text(stmt, 1, asset.id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			/* parent */
			if (not asset.parent_id.empty())
				sqlite3_bind_text(stmt, 2, asset.parent_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 2, nullptr, -1, SQLITE_STATIC);
			/* folder */
			if (not asset.folder_id.empty())
				sqlite3_bind_text(stmt, 3, asset.folder_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 3, nullptr, -1, SQLITE_STATIC);
			/* type */
			sqlite3_bind_int(stmt, 4, static_cast<int32_t>(asset.type));
			/* metadata */
			if (auto& metadata = asset.GetUserSettingsJson(); not metadata.empty())
				sqlite3_bind_text(stmt, 5, metadata.c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_text(stmt, 5, nullptr, -1, SQLITE_STATIC);
			/* createdAt */
			sqlite3_bind_int64(stmt, 6, static_cast<int64_t>(asset.GetCreatedAt()));
			/* updatedAt */
			sqlite3_bind_int64(stmt, 7, static_cast<int64_t>(asset.GetUpdatedAt()));
		});

		return DatabaseError::NoError;
	}

	DatabaseError IndexDatabase::DeleteAsset(const fig::uuid& assetID) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::DeleteAsset, [&assetID](sqlite3_stmt* stmt) {
			/* id */
			sqlite3_bind_text(stmt, 1, assetID.to_str().c_str(), -1, SQLITE_TRANSIENT);
		});
	}

	DatabaseError IndexDatabase::CreateFolder(const AssetFolder& folder) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::CreateFolder, [&folder](sqlite3_stmt* stmt) {
			/* id */
			sqlite3_bind_text(stmt, 1, folder.id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			/* parent */
			sqlite3_bind_text(stmt, 2, folder.parent_id.to_str().c_str(), -1, SQLITE_TRANSIENT);
			/* category */
			sqlite3_bind_text(stmt, 3, FolderCategoryToString(folder.category).c_str(), -1, SQLITE_TRANSIENT);
			/* name */
			sqlite3_bind_text(stmt, 4, folder.name.c_str(), -1, SQLITE_TRANSIENT);
			/* metadata */
			sqlite3_bind_text(stmt, 5, nullptr, -1, SQLITE_STATIC);
		});
	}

	DatabaseError IndexDatabase::DeleteFolder(const fig::uuid& folderID) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::DeleteFolder, [&folderID](sqlite3_stmt* stmt) {
			/* id */
			sqlite3_bind_text(stmt, 1, folderID.to_str().c_str(), -1, SQLITE_TRANSIENT);
		});
	}

	std::expected<int32_t, DatabaseError> IndexDatabase::UpsertAssets(const fig::ref_vector<Asset>& assets) noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		int32_t total_changes = 0;
		for (auto const& chunk : assets | std::views::chunk(32))
		{
			SqlTransaction transaction(_pDB);
			for (auto const& asset : chunk)
			{
				if (auto upsert_result = UpsertAsset(asset); Success(upsert_result))
				{
					LogLn(std::format("Upserted {}", (fig::string)asset.get().id));
				}
				else
				{
					LogLn(std::format("SQLError when upserting {} [parent: {}]", (fig::string)asset.get().id, (fig::string)asset.get().parent_id));
					return std::unexpected(upsert_result);
				}
			}
			
			if (auto commit_result = transaction.Commit())
				total_changes += commit_result.value();
			else
				return std::unexpected(commit_result.error());
		}

		return total_changes;
	}

	std::expected<int32_t, DatabaseError> IndexDatabase::DeleteAssets(std::span<fig::uuid> assetIDs) noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		int32_t total_changes = 0;
		for (auto const& chunk : assetIDs | std::views::chunk(32))
		{
			SqlTransaction transaction(_pDB);
			for (auto const& asset : chunk)
				DeleteAsset(asset);

			if (auto result = transaction.Commit())
				total_changes += result.value();
			else
				return std::unexpected(result.error());
		}

		return total_changes;
	}
}