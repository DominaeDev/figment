#include <pch.h>
#include "fs/AssetDatabase.h"
#include "fs/FileUtility.h"
#include <sqlite3.h>

using namespace fig::auth;
using namespace fig::util;

constexpr fig::const_string SQL_CreateTables =
	"CREATE TABLE Assets ("
	"	id        TEXT     PRIMARY KEY NOT NULL,"
	"	parent    TEXT     NOT NULL,"
	"	type      TEXT     NOT NULL,"
	"   folder    TEXT,"
	"	settings  TEXT     NOT NULL ON CONFLICT REPLACE DEFAULT [{}],"
	"	createdAt DATETIME DEFAULT (CURRENT_TIMESTAMP) NOT NULL,"
	"	updatedAt DATETIME NOT NULL DEFAULT (CURRENT_TIMESTAMP),"
	"	FOREIGN KEY (parent) REFERENCES Assets (id) ON DELETE RESTRICT ON UPDATE CASCADE,"
	"   FOREIGN KEY (folder) REFERENCES Folders (id) ON DELETE SET NULL ON UPDATE CASCADE"
	");"

	"CREATE TABLE Folders ("
	"	id       TEXT PRIMARY KEY NOT NULL,"
	"	parent   TEXT,"
	"	name     TEXT NOT NULL,"
	"	settings TEXT NOT NULL DEFAULT [{}],"
	"	FOREIGN KEY (parent) REFERENCES Folders(id) ON DELETE SET NULL ON UPDATE CASCADE"
	");";

namespace fig::io
{
	AssetDatabase::AssetDatabase(fig::path filename) :
		_filename { filename }
	{
		// Create database file
		if (not std::filesystem::exists(filename))
			CreateDatabaseAndConnect();
		else
			Connect();
	}

	AssetDatabase::~AssetDatabase()
	{
		Disconnect();
	}

	bool AssetDatabase::Connect() noexcept
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

		rc = sqlite3_exec(_pDB, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
		PrepareStatements();
		return true;
	}


	bool AssetDatabase::CreateDatabaseAndConnect() noexcept
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
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return false;
		}

		rc = sqlite3_exec(_pDB, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

		PrepareStatements();
		return true;
	}

	bool AssetDatabase::Disconnect() noexcept
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

	#define SQL_PREPARE(ENUM, SQL) sqlite3_prepare_v2(_pDB, SQL, -1, &_sqlStatements[ENUM], nullptr)

	void AssetDatabase::PrepareStatements() noexcept
	{
		// Prepare statements
		SQL_PREPARE(SQL::FetchAssets, "SELECT id, parent, type, folder, settings, createdAt, updatedAt FROM Assets;");
		SQL_PREPARE(SQL::CreateAsset, "INSERT INTO Assets (id, parent, type, folder, settings, createdAt, updatedAt) VALUES (?, ?, ?, ?, ?, ?, ?);");
		SQL_PREPARE(SQL::UpdateAsset, "UPDATE Assets SET parent = ?, type = ?, folder = ?, settings = ?, updatedAt = ? WHERE id = ?;");
		SQL_PREPARE(SQL::DeleteAsset, "DELETE FROM Assets WHERE id = ?;");
		SQL_PREPARE(SQL::CreateFolder, "INSERT INTO Folders (id, parent, category, name, settings) VALUES (?, ?, ?, ?, ?);");
		SQL_PREPARE(SQL::DeleteFolder, "DELETE FROM Folders WHERE id = ?;");
	}

	DatabaseError AssetDatabase::BindAndExecute(SQL statement, std::function<void(sqlite3_stmt*)> fnBind)
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[statement];

		fnBind(stmt);

		int rc = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		if (int nUpdates = sqlite3_changes(_pDB); !nUpdates)
		{
			Log("SQLite Error: No changes");
			return DatabaseError::ZeroChanges;
		}

		if (rc != SQLITE_DONE)
		{
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return DatabaseError::SQLError;
		}

		return DatabaseError::NoError;
	}

	std::expected<std::map<fig::uuid, Asset>, DatabaseError> AssetDatabase::FetchAssets() noexcept
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
			const char* pType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			const char* pFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
			const char* pSettings = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

			int64_t createdAt = sqlite3_column_int64(stmt, 5);
			int64_t updatedAt = sqlite3_column_int64(stmt, 6);

			fig::uuid assetID	= fig::uuid::fromStrFactory(pAssetID ? pAssetID : "");
			fig::uuid parentID	= fig::uuid::fromStrFactory(pParentID ? pParentID : "");
			fig::uuid folderID	= fig::uuid::fromStrFactory(pFolder ? pFolder : "");

			Asset asset;
			asset.id = assetID;
			asset.parent_id = parentID;
			asset.folder_id = folderID;
			auto [type, subtype] = AssetTypeFromString(pType ? pType : "");
			asset.asset_type = type;
			asset.asset_subtype = subtype;
			asset.SetMeta(MetaTag::CreatedAt, static_cast<fig::timestamp>(createdAt));
			asset.SetMeta(MetaTag::UpdatedAt, static_cast<fig::timestamp>(updatedAt));
			asset.file_status = AssetFileStatus::NotLoaded;
			asset.save_status = AssetSaveStatus::Saved;
			if (!asset.id.empty())
				result[asset.id] = std::move(asset);
		}

		sqlite3_reset(stmt);

		if (rc != SQLITE_DONE)
		{
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return std::unexpected(DatabaseError::SQLError);
		}

		return result;
	}

	std::expected<std::map<fig::uuid, AssetFolder>, DatabaseError> AssetDatabase::FetchFolders() noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		std::map<fig::uuid, AssetFolder> result;

		int rc;
		auto stmt = _sqlStatements[SQL::FetchAssets];
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			const char* pFolderID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			const char* pParentID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			const char* pCategory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			const char* pName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
			const char* pSettings = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

			fig::uuid folderID	= fig::uuid::fromStrFactory(pFolderID ? pFolderID : "");
			fig::uuid parentID	= fig::uuid::fromStrFactory(pParentID ? pParentID : "");

			AssetFolder folder;
			folder.id = folderID;
			folder.parent_id = parentID;
			if (pName)
				folder.name = fig::string { pName };
			if (pCategory)
				folder.category = FolderCategoryFromString(fig::string { pCategory });
			
			if (!folder.id.empty())
				result[folder.id] = std::move(folder);
		}

		sqlite3_reset(stmt);

		if (rc != SQLITE_DONE)
		{
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return std::unexpected(DatabaseError::SQLError);
		}

		return result;
	}

	DatabaseError AssetDatabase::CreateAsset(const Asset& asset) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::CreateAsset, [&asset](sqlite3_stmt* stmt) {
			/*id*/
			sqlite3_bind_text(stmt, 1, asset.id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*parent*/
			sqlite3_bind_text(stmt, 2, asset.parent_id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*folder*/
			sqlite3_bind_text(stmt, 3, asset.folder_id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*type*/
			sqlite3_bind_text(stmt, 4, AssetTypeToString(asset.asset_type, asset.asset_subtype).c_str(), -1, SQLITE_TRANSIENT);
			/*settings*/
			sqlite3_bind_text(stmt, 5, nullptr, -1, SQLITE_STATIC);
			/*createdAt*/
			sqlite3_bind_int64(stmt, 6, static_cast<int64_t>(asset.GetCreatedAt()));
			/*updatedAt*/
			sqlite3_bind_int64(stmt, 7, static_cast<int64_t>(asset.GetUpdatedAt()));
		});
	}

	DatabaseError AssetDatabase::UpdateAsset(const Asset& asset) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::UpdateAsset, [&asset](sqlite3_stmt* stmt) {
			/*parent*/
			sqlite3_bind_text(stmt, 1, asset.parent_id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*type*/
			sqlite3_bind_text(stmt, 2, AssetTypeToString(asset.asset_type, asset.asset_subtype).c_str(), -1, SQLITE_TRANSIENT);
			/*folder*/
			sqlite3_bind_text(stmt, 3, asset.folder_id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*settings*/
			sqlite3_bind_text(stmt, 4, nullptr, -1, SQLITE_STATIC);
			/*updatedAt*/
			sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(asset.GetUpdatedAt()));
			/*id*/
			sqlite3_bind_text(stmt, 6, asset.id.str().c_str(), -1, SQLITE_TRANSIENT);
		});

		return DatabaseError::NoError;
	}

	DatabaseError AssetDatabase::DeleteAsset(const fig::uuid& assetID) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::DeleteAsset, [&assetID](sqlite3_stmt* stmt) {
			/*id*/
			sqlite3_bind_text(stmt, 1, assetID.str().c_str(), -1, SQLITE_TRANSIENT);
		});
	}

	DatabaseError AssetDatabase::CreateFolder(const AssetFolder& folder) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::CreateFolder, [&folder](sqlite3_stmt* stmt) {
			/*id*/
			sqlite3_bind_text(stmt, 1, folder.id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*parent*/
			sqlite3_bind_text(stmt, 2, folder.parent_id.str().c_str(), -1, SQLITE_TRANSIENT);
			/*category*/
			sqlite3_bind_text(stmt, 3, FolderCategoryToString(folder.category).c_str(), -1, SQLITE_TRANSIENT);
			/*name*/
			sqlite3_bind_text(stmt, 4, folder.name.c_str(), -1, SQLITE_TRANSIENT);
			/*settings*/
			sqlite3_bind_text(stmt, 5, nullptr, -1, SQLITE_STATIC);
		});
	}

	DatabaseError AssetDatabase::DeleteFolder(const fig::uuid& folderID) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		return BindAndExecute(SQL::DeleteFolder, [&folderID](sqlite3_stmt* stmt) {
			/*id*/
			sqlite3_bind_text(stmt, 1, folderID.str().c_str(), -1, SQLITE_TRANSIENT);
		});
	}

}