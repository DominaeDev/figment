#pragma once

#include "io/Error.h"

struct sqlite3;

namespace fig::io
{
	class SqlTransaction
	{
	public:
		SqlTransaction(sqlite3* pDatabase) noexcept;
		~SqlTransaction() noexcept;

		std::expected<int32_t, DatabaseError> Commit() noexcept;
		DatabaseError GetError() const noexcept { return _error; }

	private:
		DatabaseError ExecuteStatement(const char* statement) noexcept;

		sqlite3* _pDatabase;
		DatabaseError _error = DatabaseError::NoError;
		int _prevChanges = 0;
		bool _bHasBegun = false;
		bool _bCommitted = false;
	};
}