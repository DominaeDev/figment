#include <pch.h>
#include "util/ProfileDatabase.h"
#include "fs/FileUtility.h"
#include <sqlite3.h>

using namespace fig::security;
using namespace fig::common_util;

constexpr fig::const_string create_tables =
	"CREATE TABLE Profiles("
		"id TEXT PRIMARY KEY NOT NULL,"
		"name TEXT,"
		"auth BLOB NOT NULL,"
		"salt BLOB NOT NULL,"
		"version INTEGER"
	");";

namespace fig::user
{
	ProfileDatabase::ProfileDatabase(fig::path filename) : 
		_filename { filename }
	{
		// Create database file
		if (not std::filesystem::exists(filename))
			CreateDatabaseAndConnect();
		else
			Connect();
	}

	ProfileDatabase::~ProfileDatabase()
	{
		Disconnect();
	}

	bool ProfileDatabase::Connect() noexcept
	{
		if (_pDB)
			return true; // Already connected

		if (_filename.empty())
			return false;

		if (int rc = sqlite3_open(_filename.u8string().c_str(), &_pDB); rc != SQLITE_OK)
		{
			sqlite3_close(_pDB);
			_pDB = nullptr;
			return false;
		}

		PrepareStatements();
		return true;
	}

	bool ProfileDatabase::Disconnect() noexcept
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

	void ProfileDatabase::PrepareStatements() noexcept
	{
		// Prepare statements
		SQL_PREPARE(SQL::FetchProfiles, "SELECT id, name, auth, salt, version FROM Profiles;");
		SQL_PREPARE(SQL::CreateProfile, "INSERT INTO Profiles (id, name, auth, salt, version) VALUES (?, ?, ?, ?, ?);");
		SQL_PREPARE(SQL::UpdateProfile, "UPDATE Profiles SET name = ?, auth = ?, salt = ?, version = ? WHERE id = ?;");
	}

	bool ProfileDatabase::CreateDatabaseAndConnect() noexcept
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
		rc = sqlite3_exec(_pDB, toCStr(create_tables), nullptr, nullptr, nullptr);
		if (rc != SQLITE_OK)
		{
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return false;
		}

		PrepareStatements();

		return true;
	}

	std::expected<std::vector<fig::fs::UserProfile>, DatabaseError> ProfileDatabase::FetchProfiles() noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		std::vector<fig::fs::UserProfile> result;

		int rc;
		auto stmt = _sqlStatements[SQL::FetchProfiles];
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			const char* pID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			const char* pName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			const void* auth_data = sqlite3_column_blob(stmt, 2);
			int auth_size = sqlite3_column_bytes(stmt, 2);
			const void* salt_data = sqlite3_column_blob(stmt, 3);
			int salt_size = sqlite3_column_bytes(stmt, 3);
			int version = sqlite3_column_int(stmt, 4);

			fig::uuid id = fig::uuid::fromStrFactory(pID ? pID : "");
			std::string name_str(pName ? pName : "");
			fig::bytes authChallenge {};
			fig::security::AuthSalt authSalt {};

			if (auth_data && auth_size > 0)
			{
				authChallenge.resize(toUZ(auth_size));
				std::memcpy(authChallenge.data(), auth_data, auth_size);
			}

			if (salt_data && toUZ(salt_size) == sizeof(AuthSalt))
			{
				std::memcpy(authSalt.data(), salt_data, salt_size);
			}

			fig::fs::UserProfile profile {
				.id { std::move(id) },
				.name { std::move(name_str) },
				.authChallenge { std::move(authChallenge) },
				.authSalt { std::move(authSalt) },
				.version { static_cast<unsigned short>(version) },
			};

			if (profile.IsValid())
				result.emplace_back(profile);
		}

		sqlite3_reset(stmt);

		if (rc != SQLITE_DONE)
		{
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return std::unexpected(DatabaseError::SQLError);
		}

		return result;
	}

	DatabaseError ProfileDatabase::CreateProfile(const fig::fs::UserProfile& profile) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[SQL::CreateProfile];

		sqlite3_bind_text(stmt, 1, profile.id.str().c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, profile.name.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_blob(stmt, 3, profile.authChallenge.data(), (int)profile.authChallenge.size(), SQLITE_STATIC);
		sqlite3_bind_blob(stmt, 4, profile.authSalt.data(), (int)profile.authSalt.size(), SQLITE_STATIC);
		sqlite3_bind_int(stmt, 5, profile.version);

		int rc = sqlite3_step(stmt);

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		if (rc != SQLITE_DONE)
		{
			Log(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return DatabaseError::SQLError;
		}
		return DatabaseError::NoError;
	}

	DatabaseError ProfileDatabase::UpdateProfile(const fig::fs::UserProfile& profile) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[SQL::UpdateProfile];

		sqlite3_bind_text(stmt, 1, profile.name.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_blob(stmt, 2, profile.authChallenge.data(), (int)profile.authChallenge.size(), SQLITE_STATIC);
		sqlite3_bind_blob(stmt, 3, profile.authSalt.data(), (int)profile.authSalt.size(), SQLITE_STATIC);
		sqlite3_bind_int(stmt, 4, (int)profile.version);
		sqlite3_bind_text(stmt, 5, profile.id.str().c_str(), -1, SQLITE_TRANSIENT);

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
}