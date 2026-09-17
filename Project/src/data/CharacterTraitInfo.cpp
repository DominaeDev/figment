#include <pch.h>
#include "data/CharacterTraitInfo.h"
#include "io/CsvFile.h"

using namespace fig::io;

namespace fig::data
{
	bool CharacterTraitInfoDatabase::LoadFromCsv(const fig::path& filename)
	{
		CsvFile csv;
		if (csv.Load(filename) != CsvError::NoError)
			return false;

		if (csv.GetColumnCount() != 4uz)
			return false;

		std::map<fig::string, size_t> groupIndices;
		for (auto& row : csv.GetRows())
		{
			size_t index;
			auto& categoryName = row[0];
			if (groupIndices.contains(categoryName))
				index = groupIndices[categoryName];
			else
			{
				index = groupIndices[categoryName] = groups.size();
				groups.emplace_back(TraitGroup {
					.name = categoryName,
				});
			}
			
			auto& group = groups[index];

			group.traits.emplace_back(CharacterTraitInfo {
				.id = fig::handle { row[1] },
				.name = row[1],
				.text = row[2],
				.visibility = enum_deserialize(row[3], CharacterTrait::VisibilityMapping),
			});
		}

		return true;
	}
}