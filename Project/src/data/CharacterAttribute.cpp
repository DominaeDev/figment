#include <pch.h>
#include "data/CharacterAttribute.h"

namespace fig::data
{
	const std::map<CharacterAttribute::ValueType, fig::string> CharacterAttribute::ValueTypeMapping {
		{ CharacterAttribute::ValueType::ShortText,		"text" },
		{ CharacterAttribute::ValueType::LongText,		"multiline" },
		{ CharacterAttribute::ValueType::Number,		"number" },
		{ CharacterAttribute::ValueType::List,			"list" },
		{ CharacterAttribute::ValueType::Options,		"options" },
	};

	const std::map<CharacterAttribute::Visibility, fig::string> CharacterAttribute::VisibilityMapping {
		{ CharacterAttribute::Visibility::Public,		"public" },
		{ CharacterAttribute::Visibility::Private,		"private" }
	};

	const std::map<CharacterAttribute::HintFlag, fig::string> CharacterAttribute::FlagMapping {
		{ CharacterAttribute::HintFlag::Trivial,		"trivial" },
		{ CharacterAttribute::HintFlag::Important,		"important" },
		{ CharacterAttribute::HintFlag::Memory,			"memory" }
	};

}