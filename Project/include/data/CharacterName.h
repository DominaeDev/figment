#pragma once

#include "Figment.h"

namespace fig::data
{
	struct CharacterName
	{
		fig::string first;
		fig::string last;
		fig::string nickname;

		fig::string GetSpokenName() const;
		fig::string GetFullName() const;
		bool empty() const;

		static auto XmlFields() noexcept
		{
			return Fields(
				Element { "First", &CharacterName::first },
				Element { "Last", &CharacterName::last },
				Element { "Nickname", &CharacterName::nickname }
			);

			static_assert(IsXmlSerializable<CharacterName>);
		}
	};
}
