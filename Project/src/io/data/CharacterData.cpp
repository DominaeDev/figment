#include <pch.h>
#include "io/data/CharacterData.h"
#include "io/Xml.h"

using namespace fig::gui;
using namespace fig::io;

namespace fig::io
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
			XmlElement { "Brief",			&CharacterData::brief },
			XmlElement { "Description",		&CharacterData::description },
			XmlElement { "Tags",			&CharacterData::tags },
			XmlElement { "Gender",			&CharacterData::gender, XmlConvertString<CharacterGender> },
			XmlElement { "SearchIndex",		&CharacterData::searchIndex, 
				[](auto& value) -> fig::string { return value.Serialize(); },
				[](auto& value) -> SearchIndex { SearchIndex s; s.Deserialize(value); return s; }
			}
		);
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

		// Read gender
		if (data.gender.IsDefined())
		{
			data.properties[Constants::CharacterProperties::Gender] = CharacterProperty {
				.label = "Gender",
				.value = data.gender,
			};
		}

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
}