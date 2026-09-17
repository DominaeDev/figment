#include <pch.h>
#include "io/CsvFile.h"

namespace fig::io
{
	const CsvFile::CsvRow& CsvFile::GetRow(size_t index) const
	{
		return _rows[index];
	}

	CsvError CsvFile::Load(const fig::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (not file.is_open())
			return CsvError::FileNotFound;

		fig::string line;
		bool headerParsed = false;

		while (std::getline(file, line))
		{
			if (not line.empty() and line.back() == '\r')
				line.pop_back();

			auto fields = decode_csv(line);

			if (not headerParsed)
			{
				_columnCount = fields.size();
				headerParsed = true;
				continue;
			}

			_rows.push_back(std::move(fields));
		}

		if (file.bad())
			return CsvError::FileReadError;

		return CsvError::NoError;
	}

}