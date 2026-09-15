#pragma once

#include "gui/GUITypes.h"
#include "data/CharacterName.h"
#include "data/CharacterGender.h"
#include "data/CharacterAttribute.h"
#include "data/CharacterTrait.h"
#include "util/SearchIndex.h"
#include "text/Context.h"

namespace fig::data
{
	class Character
	{
	public:
		fig::io::FileError LoadFromXml(const fig::path& filename);
		fig::io::FileError LoadFromXml(const fig::string& doc);
		fig::io::FileError LoadFromXml(fig::string_view doc);
		void SaveToXml(fig::bytes& buffer) const;

		std::optional<CharacterAttribute> FindAttribute(const fig::handle& attributeId) const noexcept;
		std::optional<fig::string> GetAttribute(const fig::handle& attributeId) const noexcept;
		const std::vector<CharacterAttribute>& GetAttributes() const noexcept { return _attributes; }
		const CharacterAttribute& SetAttribute(const fig::handle& attributeId, fig::string_view name, fig::string_view content, CharacterAttribute::ValueType format = CharacterAttribute::ValueType::ShortText, CharacterAttribute::Visibility visibility = CharacterAttribute::Visibility::Public, CharacterAttribute::HintFlags flags = {});
		bool RemoveAttribute(const fig::handle& attributeId);
		void ClearAttributes();

		bool HasTrait(const fig::handle& traitId) const noexcept;
		std::optional<CharacterTrait> FindTrait(const fig::handle& traitId) const noexcept;
		const std::vector<CharacterTrait>& GetTraits() const noexcept { return _traits; }
		const CharacterTrait& SetTrait(const fig::handle& traitId, fig::string_view name, fig::string_view content, CharacterTrait::Visibility visibility = CharacterTrait::Visibility::Public);
		bool RemoveTrait(const fig::handle& traitId);
		void ClearTraits();

		void SetTags(const fig::string_list& tags) noexcept;
		void AppendTags(const fig::string_list& tags);
		const fig::string_list& GetTags() const noexcept { return _tags; }

		void AddSearchTerm(const fig::string& term);
		const SearchIndex& GetSearchIndex() const noexcept { return _searchIndex; }

		[[nodiscard]] const Context& GetContext() noexcept;

		fig::string GetName() const noexcept { return name.GetSpokenName(); }
		fig::string GetFullName() const noexcept { return name.GetFullName(); }
		fig::string GetDescription() const noexcept;

	public:
		fig::string chatId;	//! @remove?
		CharacterName name;
		Gender gender {};
		Pronouns pronouns { Pronouns::Undefined };
		fig::string brief;
		
		fig::color bgColor {};
		fig::color borderColor {};

		// Meta data
		fig::string creator;
		fig::string about; // User-facing only
		fig::string version;
	
	private:
		void UpdateContext();

		std::vector<CharacterAttribute> _attributes;
		std::vector<CharacterTrait> _traits;
		fig::string_list _tags;
		SearchIndex _searchIndex;
		Context _context;
		bool _bDirtyContext {};

	public:
		static auto XmlFields() noexcept;
	};
}
