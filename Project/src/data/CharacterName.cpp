#include <pch.h>
#include "data/CharacterName.h"
#include "io/Xml.h"

namespace fig::data
{
	fig::string CharacterName::GetSpokenName() const
	{
		if (not nickname.empty())
			return nickname;
		if (not first.empty())
			return first;
		return "Unnamed"; //! @todo
	}

	fig::string CharacterName::GetFullName() const
	{
		if (not (first.empty() or last.empty()))
			return std::format("{} {}", first, last);
		if (not first.empty())
			return first;
		if (not nickname.empty())
			return nickname;
		return "Unnamed"; //! @todo
	}

	bool CharacterName::empty() const
	{
		return first.empty() and nickname.empty();
	}
}