#pragma once

#include "Figment.h"
#include "io/Xml.h"
#include "io/XmlData.h"

namespace fig::data
{
	using WorldRule = fig::string; // for now

	class World : public XmlData<"World", 0>
	{
	public:
		void SetRules(const std::vector<WorldRule>& rules) noexcept;
		void AppendRules(const std::vector<WorldRule>& rules) noexcept;
		void ClearRules() noexcept;
		const std::vector<WorldRule>& GetRules() const noexcept { return _rules; }

		void SetTags(const fig::string_list& tags) noexcept;
		void AppendTags(const fig::string_list& tags);
		const fig::string_list& GetTags() const noexcept { return _tags; }

		fig::string name;
		fig::string about;
		fig::string creator;
		fig::string version;
		std::vector<WorldRule> _rules;
		fig::string_list _tags;
		// @todo: Concepts

	public:
		static auto XmlFields() noexcept
		{
			using namespace fig::io;

			return Fields(
				Element { "Name",			&World::name },
				Element { "Description",	&World::about },
				Element { "Creator",		&World::creator },
				Element { "Version",		&World::version },
				Element { "Tags",			&World::_tags },
				Element { "Rule",			&World::_rules }
					.Collection("Rules")
			);

			static_assert(IsXmlSerializable<World>);
		}
	};
}
