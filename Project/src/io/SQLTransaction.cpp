#include <pch.h>
#include <sqlite3.h>
#include "io/SQLTransaction.h"

namespace fig::io
{
	SqlTransaction::SqlTransaction(sqlite3* pDatabase) noexcept :
		_pDatabase(pDatabase)
	{
		_prevChanges = sqlite3_total_changes(_pDatabase);
		_error = ExecuteStatement("BEGIN");
		_bHasBegun = (_error == DatabaseError::NoError);
	}

	SqlTransaction::~SqlTransaction() noexcept
	{
		if (_bHasBegun and not _bCommitted)
			ExecuteStatement("ROLLBACK");
	}

	std::expected<int32_t, DatabaseError> SqlTransaction::Commit() noexcept
	{
		if (not _bHasBegun)
			return std::unexpected(_error);

		_error = ExecuteStatement("COMMIT");
		if (_error == DatabaseError::NoError)
		{
			_bCommitted = true;
			return sqlite3_total_changes(_pDatabase) - _prevChanges;
		}
		return std::unexpected(_error);
	}

	DatabaseError SqlTransaction::ExecuteStatement(const char* statement) noexcept
	{
		char* pErrorMessage = nullptr;
		int returnCode = sqlite3_exec(_pDatabase, statement, nullptr, nullptr, &pErrorMessage);
		if (returnCode != SQLITE_OK)
		{
			Log(std::format("SQLite Error: {}", pErrorMessage ? pErrorMessage : "unknown"));
			sqlite3_free(pErrorMessage);
			return DatabaseError::SQLError;
		}
		return DatabaseError::NoError;
	}
}