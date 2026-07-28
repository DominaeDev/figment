#include <pch.h>
#include "user/ProfileDatabase.h"
#include "io/FileUtility.h"
#include <sqlite3.h>

using namespace fig::auth;
using namespace fig::user;

static constexpr fig::const_string SQL_CreateTable =
	"CREATE TABLE IF NOT EXISTS Profiles("
		"id         TEXT     PRIMARY KEY NOT NULL,"
		"version    INTEGER  NOT NULL,"
		"name       TEXT     NOT NULL,"
		"state      INTEGER  NOT NULL DEFAULT (0),"
		"auth       BLOB     NOT NULL,"
		"recovery   BLOB     NOT NULL"
	");";

namespace fig::io
{
	ProfileDatabase::ProfileDatabase(fig::path filename) : 
		_filename { filename }
	{
		Connect();
	}

	ProfileDatabase::~ProfileDatabase()
	{
		Disconnect();
	}

	DatabaseError ProfileDatabase::Connect() noexcept
	{
		if (IsConnected())
			return DatabaseError::NoError; // Already connected

		// Create database if it doesn't exist
		if (not std::filesystem::exists(_filename))
		{
			// Create empty file
			std::ofstream fs(_filename.u8string().c_str(), std::ios::binary | std::ios::out);
			if (not fs.is_open())
				return DatabaseError::NotConnected;
			fs.close();
		}

		int rc = sqlite3_open(_filename.u8string().c_str(), &_pDB);
		if (rc != SQLITE_OK)
		{
			sqlite3_close(_pDB);
			_pDB = nullptr;
			return DatabaseError::NotConnected;
		}

		// Create table(s)
		rc = sqlite3_exec(_pDB, toCStr(SQL_CreateTable), nullptr, nullptr, nullptr);
		if (rc != SQLITE_OK)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			assert(false && "SQLite error");
			return DatabaseError::SQLError;
		}

		PrepareStatements();
		return DatabaseError::NoError;
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

	void ProfileDatabase::PrepareStatements() noexcept
	{
		auto Prepare = [this](SQL op, fig::string_view sql) {
			int rc = sqlite3_prepare_v2(_pDB, sql.data(), static_cast<int32_t>(sql.length()), &_sqlStatements[op], nullptr);
			assert(rc == SQLITE_OK);
		};

		// Prepare statements
		Prepare(SQL::FetchProfiles, "SELECT id, version, name, state, auth, recovery FROM Profiles;");
		Prepare(SQL::CreateProfile, "INSERT INTO Profiles (id, version, name, state, auth, recovery) VALUES (?, ?, ?, ?, ?, ?);");
		Prepare(SQL::UpdateProfile, "UPDATE Profiles SET version = ?, name = ?, state = ?, auth = ?, recovery = ? WHERE id = ?;");
		Prepare(SQL::UpdateRecovery, "UPDATE Profiles SET recovery = ? WHERE id = ?;");
	}

	std::expected<std::vector<fig::user::UserProfile>, DatabaseError> ProfileDatabase::FetchProfiles() noexcept
	{
		if (!_pDB)
			return std::unexpected(DatabaseError::NotConnected);

		std::vector<fig::user::UserProfile> result;

		int rc;
		auto stmt = _sqlStatements[SQL::FetchProfiles];
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			/* id */
			const char* pID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			/* version */
			int version = sqlite3_column_int(stmt, 1);
			/* name */
			const char* pName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			/* state */
			bool state = sqlite3_column_int(stmt, 3) != 0;
			/* auth */
			const void* auth_data = sqlite3_column_blob(stmt, 4);
			int auth_size = sqlite3_column_bytes(stmt, 4);
			/* recovery */
			const void* recovery_data = sqlite3_column_blob(stmt, 5);
			int recovery_size = sqlite3_column_bytes(stmt, 5);

			fig::uuid id = fig::uuid::from_str(pID ? pID : "");
			fig::string name_str(pName ? pName : "");
			fig::auth::UserAuth userAuth {};
			fig::auth::UserAuth recoveryData {};

			if (auth_data && toUZ(auth_size) == sizeof(fig::auth::UserAuth))
				std::memcpy(&userAuth, auth_data, sizeof(fig::auth::UserAuth));
			if (recovery_data && toUZ(recovery_size) == sizeof(fig::auth::UserAuth))
				std::memcpy(&recoveryData, recovery_data, sizeof(fig::auth::UserAuth));

			fig::user::UserProfile profile {
				.version { static_cast<AuthVersion>(version) },
				.id { std::move(id) },
				.name { std::move(name_str) },
				.auth { std::move(userAuth) },
				.recovery { std::move(recoveryData) },
				.state { static_cast<UserProfile::State>(state) },
			};

			if (profile.IsValid())
				result.emplace_back(profile);
		}

		sqlite3_reset(stmt);

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return std::unexpected(DatabaseError::SQLError);
		}

		return result;
	}

	DatabaseError ProfileDatabase::CreateProfile(const fig::user::UserProfile& profile) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[SQL::CreateProfile];

		/* id */
		sqlite3_bind_text(stmt, 1, profile.id.to_str().c_str(), -1, SQLITE_TRANSIENT);
		/* version */
		sqlite3_bind_int(stmt, 2, profile.version);
		/* name */
		sqlite3_bind_text(stmt, 3, profile.name.c_str(), -1, SQLITE_STATIC);
		/* state */
		sqlite3_bind_int(stmt, 4, static_cast<int32_t>(profile.state));
		/* auth */
		sqlite3_bind_blob(stmt, 5, &profile.auth, sizeof(fig::auth::UserAuth), SQLITE_STATIC);
		/* recovery */
		sqlite3_bind_blob(stmt, 6, &profile.recovery, sizeof(fig::auth::UserAuth), SQLITE_STATIC);

		int rc = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return DatabaseError::SQLError;
		}
		return DatabaseError::NoError;
	}

	DatabaseError ProfileDatabase::UpdateProfile(const fig::user::UserProfile& profile) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[SQL::UpdateProfile];

		/* version */
		sqlite3_bind_int(stmt, 1, (int)profile.version);
		/* name */
		sqlite3_bind_text(stmt, 2, profile.name.c_str(), -1, SQLITE_STATIC);
		/* state */
		sqlite3_bind_int(stmt, 3, static_cast<int32_t>(profile.state));
		/* auth */
		sqlite3_bind_blob(stmt, 4, &profile.auth, sizeof(fig::auth::UserAuth), SQLITE_STATIC);
		/* recovery */
		sqlite3_bind_blob(stmt, 5, &profile.recovery, sizeof(fig::auth::UserAuth), SQLITE_STATIC);
		/* id */
		sqlite3_bind_text(stmt, 6, profile.id.to_str().c_str(), -1, SQLITE_TRANSIENT);

		int rc = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		if (int nUpdates = sqlite3_changes(_pDB); !nUpdates)
		{
			LogLn("SQLite Error: No changes");
			return DatabaseError::ZeroChanges;
		}

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return DatabaseError::SQLError;
		}
		return DatabaseError::NoError;
	}

	DatabaseError ProfileDatabase::UpdateRecovery(const fig::user::UserProfile& profile) noexcept
	{
		if (!_pDB)
			return DatabaseError::NotConnected;

		auto stmt = _sqlStatements[SQL::UpdateRecovery];

		/* recovery */
		sqlite3_bind_blob(stmt, 1, &profile.recovery, sizeof(fig::auth::UserAuth), SQLITE_STATIC);
		/* id */
		sqlite3_bind_text(stmt, 2, profile.id.to_str().c_str(), -1, SQLITE_TRANSIENT);

		int rc = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		if (int nUpdates = sqlite3_changes(_pDB); !nUpdates)
		{
			LogLn("SQLite Error: No changes");
			return DatabaseError::ZeroChanges;
		}

		if (rc != SQLITE_DONE)
		{
			LogLn(std::format("SQLite Error: {}", sqlite3_errmsg(_pDB)));
			return DatabaseError::SQLError;
		}
		return DatabaseError::NoError;
	}
}