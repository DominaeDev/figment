#include <pch.h>
#include "data/CharacterData.h"
#include "io/Xml.h"

using namespace fig::gui;
using namespace fig::io;

namespace fig::data
{
	static const fig::string XmlRootName { "Character" };

	auto CharacterData::GetXmlFields()
	{
		return XmlFields(
			XmlElement { "ID",				&CharacterData::chatId  }
				.MustExist(),
			XmlElement { "ShortName",		&CharacterData::shortName }
				.MustExist(),
			XmlElement { "FullName",		&CharacterData::fullName },
			XmlElement { "Gender",			&CharacterData::gender, XmlConvertString<CharacterGender> },
			XmlElement { "Brief",			&CharacterData::brief },
			XmlElement { "Attributes",		&CharacterData::_attributes },
			XmlElement { "Tags",			&CharacterData::_tags },
			XmlElement { "SearchIndex",		&CharacterData::_searchIndex,
				[](auto& value) -> fig::string { return value.Serialize(); },
				[](auto& value) -> SearchIndex { SearchIndex s; s.Deserialize(value); return s; }
			}
		);

		static_assert(XmlSerializable<CharacterData>);
	}

	static const std::map<CharacterAttribute::Format, fig::string> FormatMapping {
		{ CharacterAttribute::Format::Text,		"text" },
		{ CharacterAttribute::Format::Number,	"number" },
		{ CharacterAttribute::Format::List,		"list" },
	};

	static const std::map<CharacterAttribute::Visibility, fig::string> VisibilityMapping {
		{ CharacterAttribute::Visibility::Public,	"public" },
		{ CharacterAttribute::Visibility::Private,	"private" }
	};

	auto CharacterAttribute::GetXmlFields()
	{
		return XmlFields(
			XmlAttribute { "format", &CharacterAttribute::format, 
				[](auto& value) { return enum_serialize(value, FormatMapping); }, 
				[](auto& value) { return enum_deserialize(value, FormatMapping); } 
			},
			XmlAttribute { "visibility", &CharacterAttribute::visibility, 
				[](auto& value) { return enum_serialize(value, VisibilityMapping); },
				[](auto& value) { return enum_deserialize(value, VisibilityMapping); }
			},
			XmlElement { "Label", &CharacterAttribute::label }.MustExist(),
			XmlElement { "Value", &CharacterAttribute::value }.MustExist()
		);

		static_assert(XmlSerializable<CharacterAttribute>);
		static_assert(XmlSerializableMap<std::map<fig::string, CharacterAttribute>>);
	}

	static bool ReadXml(XmlReader& xml, CharacterData& data)
	{
		auto rootNode = xml.GetRoot();

		XmlDeserialize(rootNode, data);

		if (data.shortName.empty())
			data.shortName = data.fullName;
		if (data.fullName.empty())
			data.fullName = data.shortName;
		if (data.chatId.empty())
			data.chatId = data.shortName;

		// Colors
		data.bgColor = {};
		data.borderColor = {};

		if (auto colorText = rootNode.TryGetElement<fig::string>("Color"))
		{
			data.borderColor = Color::FromString(colorText.value());

			auto [h, s, v] = data.borderColor.GetHSV();

			if (s > 0.0f)
				data.bgColor = Color::FromHSV(h, 0.05f, std::clamp(v + 0.25f, 0.8f, 1.0f));
			else
				data.bgColor = Color::FromHSV(h, 0.0f, std::clamp(v + 0.5f, 0.8f, 1.0f));
		}

		return !data.chatId.empty() && !data.shortName.empty();
	}

	FileError CharacterData::LoadFromXml(const fig::path& path)
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::NotFound;

		XmlReader xml(path, XmlRootName);
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError CharacterData::LoadFromXml(const fig::string& doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRoot().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError CharacterData::LoadFromXml(fig::string_view doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRoot().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	void CharacterData::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml(XmlRootName);

		auto root = xml.GetRoot();

		XmlSerialize(root, *this);
		
		xml.WriteToMemory(buffer);
	}

	std::optional<CharacterAttribute> CharacterData::FindAttribute(const fig::string_view& attributeId) const noexcept
	{
		if (auto itFind = _attributes.find(lcase(toStr(attributeId))); itFind != _attributes.cend())
			return itFind->second;
		return std::nullopt;
	}

	std::optional<fig::string> CharacterData::GetAttribute(const fig::string_view& attributeId) const noexcept
	{
		if (auto try_attrib = FindAttribute(attributeId))
			return try_attrib.value().value;
		return "";
	}

	void CharacterData::AppendTags(const fig::string_list& tags)
	{
		_tags.append_range(tags);
		_searchIndex.AddTerms(tags);
	}

	void CharacterData::AddSearchTerm(const fig::string& term)
	{
		_searchIndex.AddTerm(term);
	}

	void CharacterData::AddAttribute(const fig::string& attributeId, const fig::string& label, const fig::string& content, CharacterAttribute::Format format, CharacterAttribute::Visibility visibility)
	{
		_attributes[lcase(attributeId)] = CharacterAttribute {
			.label = label,
			.value = content,
			.format = format,
			.visibility = visibility,
		};
	}
}