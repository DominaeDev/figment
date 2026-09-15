#include <pch.h>
#include "data/Character.h"

using namespace fig::gui;
using namespace fig::io;

namespace fig::data
{
	static const fig::string XmlRootName { "Character" };

	auto Character::XmlFields() noexcept
	{
		return Fields(
			Element { "ID", &Character::chatId },
			Element { "Name", &Character::name }
				.MustExist(),
			Element { "Gender", &Character::gender,
				[](auto& value) { return (fig::string)value; },
				[](auto& value) { return Gender(value); }
			},
			Element { "Brief", &Character::brief },
			Element { "Attributes", &Character::_attributes },
			Element { "Traits", &Character::_traits },
			Element { "Tags", &Character::_tags },
			Element { "SearchIndex", &Character::_searchIndex,
				[](auto& value) -> fig::string { return value.Serialize(); },
				[](auto& value) -> SearchIndex { SearchIndex s; s.Deserialize(value); return s; }
			},
			Element { "About", &Character::about },
			Element { "Creator", &Character::creator },
			Element { "Version", &Character::version }
		);

		static_assert(IsXmlSerializable<Character>);
	}

	static bool ReadXml(XmlReader& xml, Character& data)
	{
		auto rootNode = xml.GetRoot();

		if (!Deserialize(rootNode, data))
			return false;

		if (data.chatId.empty())
			data.chatId = data.name.GetSpokenName();

		// Colors
		data.bgColor = {};
		data.borderColor = {};

		if (auto colorText = rootNode.TryGetElement<fig::string>("Color"))
		{
			data.borderColor = fig::color::FromString(colorText.value());

			auto [h, s, v] = data.borderColor.GetHSV();

			if (s > 0.0f)
				data.bgColor = fig::color::FromHSV(h, 0.05f, std::clamp(v + 0.25f, 0.8f, 1.0f));
			else
				data.bgColor = fig::color::FromHSV(h, 0.0f, std::clamp(v + 0.5f, 0.8f, 1.0f));
		}

		return not (data.chatId.empty() or data.name.empty());
	}

	FileError Character::LoadFromXml(const fig::path& path)
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::NotFound;

		XmlReader xml(path, XmlRootName);
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat;

		_bDirtyContext = true;
		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError Character::LoadFromXml(const fig::string& doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRoot().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		_bDirtyContext = true;
		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError Character::LoadFromXml(fig::string_view doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRoot().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		_bDirtyContext = true;
		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	void Character::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml(XmlRootName);

		auto root = xml.GetRoot();

		Serialize(root, *this);
		
		xml.WriteToMemory(buffer);
	}

	std::optional<CharacterAttribute> Character::FindAttribute(const fig::handle& attributeId) const noexcept
	{
		if (auto itFind = std::ranges::find_if(_attributes, [&attributeId](auto&& a) { return a.id == attributeId; }); itFind != std::ranges::cend(_attributes))
			return *itFind;
		return std::nullopt;
	}

	std::optional<fig::string> Character::GetAttribute(const fig::handle& attributeId) const noexcept
	{
		if (auto try_attrib = FindAttribute(attributeId))
			return try_attrib.value().value;
		return std::nullopt;
	}

	const CharacterAttribute& Character::SetAttribute(const fig::handle& attributeId, fig::string_view label, fig::string_view content, CharacterAttribute::ValueType format, CharacterAttribute::Visibility visibility, CharacterAttribute::HintFlags flags)
	{
		fig::handle id = attributeId;
		if (id.empty())
			id = fig::handle { label };

		// Update existing
		if (auto try_attribute = FindAttribute(id))
		{
			auto& attribute = try_attribute.value();
			attribute.name = fig::string { label };
			attribute.value = fig::string { content };
			attribute.type = format;
			attribute.visibility = visibility;
			attribute.flags = flags;
			return attribute;
		}

		// Add new
		_attributes.emplace_back(CharacterAttribute {
			.id = id,
			.name = fig::string { label },
			.value = fig::string { content },
			.type = format,
			.visibility = visibility,
			.flags = flags,
		});
		_bDirtyContext = true;
		return _attributes.back();
	}

	bool Character::RemoveAttribute(const fig::handle& attributeId)
	{
		if (auto e = std::ranges::remove(_attributes, attributeId, [](auto&& a) { return a.id; }); e.begin() != e.end())
		{
			_attributes.erase(e.begin(), e.end());
			return true;
		}
		return false;
	}

	void Character::ClearAttributes()
	{
		_attributes.clear();
	}

	bool Character::HasTrait(const fig::handle& traitId) const noexcept
	{
		return std::ranges::contains(_traits, traitId, [](auto&& t) { return t.id; });
	}

	std::optional<CharacterTrait> Character::FindTrait(const fig::handle& traitId) const noexcept
	{
		if (auto itFind = std::ranges::find_if(_traits, [&traitId](auto&& t) { return t.id == traitId; }); itFind != std::ranges::cend(_traits))
			return *itFind;
		return std::nullopt;
	}

	const CharacterTrait& Character::SetTrait(const fig::handle& traitId, fig::string_view name, fig::string_view text, CharacterTrait::Visibility visibility)
	{
		fig::handle id = traitId;
		if (id.empty())
			id = fig::handle { name };

		// Update existing
		if (auto try_trait = FindTrait(id))
		{
			auto& trait = try_trait.value();
			trait.name = fig::string { name };
			trait.text = fig::string { text };
			trait.visibility = visibility;
			return trait;
		}

		// Add new
		_traits.emplace_back(CharacterTrait {
			.id = id,
			.name = fig::string { name },
			.text = fig::string { text },
			.visibility = visibility,
		});
		_bDirtyContext = true;
		return _traits.back();
	}

	bool Character::RemoveTrait(const fig::handle& traitId)
	{
		if (auto e = std::ranges::remove(_traits, traitId, [](auto&& a) { return a.id; }); e.begin() != e.end())
		{
			_traits.erase(e.begin(), e.end());
			return true;
		}
		return false;
	}

	void Character::ClearTraits()
	{
		_traits.clear();
	}

	void Character::AppendTags(const fig::string_list& tags)
	{
		_tags.append_range(tags);
		_searchIndex.AddTerms(tags);
		_bDirtyContext = true;
	}

	void Character::AddSearchTerm(const fig::string& term)
	{
		_searchIndex.AddTerm(term);
	}

	const Context& Character::GetContext() noexcept
	{
		if (_bDirtyContext)
			UpdateContext();
		return _context;
	}

	void Character::UpdateContext()
	{
		_context.Clear();
		_context.SetValue("id", chatId);
		_context.SetValue("name", name.GetSpokenName());
		_context.SetValue("fullname", name.GetFullName());
		_context.SetValue("gender", fig::string { gender });
		_context.SetValue("brief", brief);

		for (auto& attrib : _attributes)
			_context.SetValue(attrib.id, attrib.value);

		if (gender.IsConventional())
			_context.SetFlag((fig::string)gender);

		_bDirtyContext = false;
	}

	fig::string Character::GetDescription() const noexcept
	{
		if (not about.empty())
			return truncate(strip_emoji(about), 1024);
		return brief;
	}

	void Character::SetTags(const fig::string_list& tags) noexcept
	{
		_tags = tags;
	}
}