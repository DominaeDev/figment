#include <pch.h>
#include "model/CharacterData.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"

using namespace fig::gui;
using namespace fig::io;

namespace fig::io
{
	static const fig::string XmlRootName { "Character" };

	auto CharacterData::XmlFields()
	{
		return std::make_tuple(
			XmlElement { &CharacterData::chatId, "ID" }
				.MustExist(),
			XmlElement { &CharacterData::shortName, "ShortName" }
				.MustExist(),
			XmlElement { &CharacterData::fullName, "FullName" },
			XmlElement { &CharacterData::brief, "Brief" },
			XmlElement { &CharacterData::description, "Description" },
			XmlElement { &CharacterData::tags, "Tags" },
			XmlElement { &CharacterData::gender, "Gender",
				[](auto& g) { return g.GetName(); },
				[](auto& s) { return CharacterGender { s }; }
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
				.value = data.gender.GetName(),
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

		// Tags
		data.tags = rootNode.TryGetElement<fig::string_list>("Tags").value_or({});

		// Search
		data.searchIndex.Deserialize(rootNode.TryGetElement<fig::string>("SearchIndex").value_or(""));

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
		
		if (not searchIndex.IsEmpty())
			root.SetElementValue("SearchIndex", searchIndex.Serialize());

		xml.WriteToMemory(buffer);
	}
}