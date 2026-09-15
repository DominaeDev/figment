#include <pch.h>
#include "data/CharacterTrait.h"

namespace fig::data
{
	const std::map<CharacterTrait::Visibility, fig::string> CharacterTrait::VisibilityMapping {
		{ CharacterTrait::Visibility::Public,	"public" },
		{ CharacterTrait::Visibility::Private,	"private" }
	};
}