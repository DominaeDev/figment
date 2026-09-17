#pragma once

#include "Figment.h"

namespace fig::io
{
	enum class CsvError
	{
		NoError = 0,
		FileNotFound,
		FileReadError,
		ReadError,
	};

	class CsvFile
	{
	public:
		using CsvRow = std::vector<fig::string>;

		size_t GetColumnCount() const noexcept { return _columnCount; }
		size_t GetRowCount() const noexcept { return _rows.size(); }

		const CsvRow& GetRow(size_t index) const;
		const std::vector<CsvRow>& GetRows() const { return _rows; }

		CsvError Load(const fig::path& path);

	private:
		size_t _columnCount {};
		std::vector<CsvRow> _rows;
	};
}