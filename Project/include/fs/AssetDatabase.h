#ifndef ASSET_DATABASE_H__
#define ASSET_DATABASE_H__
#pragma once

#include "Types.h"
#include "fs/DatabaseError.h"
#include "model/Asset.h"
#include <expected>
#include <functional>

struct sqlite3;
struct sqlite3_stmt;

namespace fig::io
{
	class AssetDatabase
	{
	public:
		AssetDatabase(fig::path filename);
		virtual ~AssetDatabase();

		std::expected<std::map<fig::uuid, AssetFolder>, DatabaseError> FetchFolders() noexcept;
		std::expected<std::map<fig::uuid, Asset>, DatabaseError> FetchAssets() noexcept;
		DatabaseError CreateAsset(const Asset& asset) noexcept;
		DatabaseError UpdateAsset(const Asset& asset) noexcept;
		DatabaseError DeleteAsset(const fig::uuid& assetID) noexcept;
		
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

#endif