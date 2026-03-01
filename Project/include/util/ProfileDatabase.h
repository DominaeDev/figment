#ifndef PROFILE_DATABASE_H__
#define PROFILE_DATABASE_H__
#pragma once

#include "Types.h"
#include "model/UserProfile.h"
#include "fs/DatabaseError.h"
#include <expected>

struct sqlite3;
struct sqlite3_stmt;

namespace fig::io
{
	class ProfileDatabase
	{
	public:
		ProfileDatabase(fig::path filename);
		virtual ~ProfileDatabase();

		std::expected<std::vector<fig::user::UserProfile>, DatabaseError> FetchProfiles() noexcept;
		DatabaseError CreateProfile(const fig::user::UserProfile& profile) noexcept;
		DatabaseError UpdateProfile(const fig::user::UserProfile& profile) noexcept;
		DatabaseError UpdateRecovery(const fig::user::UserProfile& profile) noexcept;
		DatabaseError DeleteProfile(const fig::uuid& id) noexcept;
		
		bool IsConnected() const noexcept { return _pDB != nullptr; }

	private:
		bool Connect() noexcept;
		bool Disconnect() noexcept;
		bool CreateDatabaseAndConnect() noexcept;
		void PrepareStatements() noexcept;

	private:
		fig::path _filename;
		sqlite3* _pDB = nullptr;

		enum class SQL
		{
			CreateProfileTable,
			FetchProfiles,
			CreateProfile,
			UpdateProfile,
			UpdateRecovery,
		};
		std::map<SQL, sqlite3_stmt*> _sqlStatements;
	};
}

#endif