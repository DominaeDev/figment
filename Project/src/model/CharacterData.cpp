#include <pch.h>
#include "model/CharacterData.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"

using namespace fig::gui;
using namespace fig::util;
using namespace fig::io;

namespace fig::io
{
	static const fig::string XmlRootName { "Character" };

	static bool ReadXml(XmlReader& xml, CharacterData& data)
	{
		auto rootNode = xml.GetRootElement();

		XmlDeserialize(rootNode, data);

		if (data.shortName.empty())
			data.shortName = data.fullName;
		if (data.fullName.empty())
			data.fullName = data.shortName;
		if (data.chatId.empty())
			data.chatId = data.shortName;

		// Read gender
		if (auto genderText = rootNode.TryGetElement<fig::string>("Gender"))
		{
			fig::string gender = trim(genderText.value());

			if (equals(gender, "male", true))
				data.gender = CharacterGender::Male;
			else if (equals(gender, "female", true))
				data.gender = CharacterGender::Female;
			else if (not gender.empty())
			{
				data.gender = CharacterGender::Custom;
				data.properties[Constants::CharacterProperties::Gender] = CharacterProperty { 
					.label = "Gender", 
					.value = gender 
				};
			}
			else
				data.gender = CharacterGender::Undefined;
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
		if (not xml.IsOk() or xml.GetRootElement().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError CharacterData::LoadFromXml(fig::string_view doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRootElement().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	void CharacterData::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml(XmlRootName);

		auto root = xml.GetRoot();

		XmlSerialize(root, *this);
		
		switch (gender)
		{
		case CharacterGender::Male:
			root.SetElementValue("Gender", "Male");
			break;
		case CharacterGender::Female:
			root.SetElementValue("Gender", "Female");
			break;
		case CharacterGender::Custom:
			if (auto it = properties.find(Constants::CharacterProperties::Gender); it != properties.cend())
				root.SetElementValue("Gender", it->second.value);
			break;
		default:
			break;
		}

		if (not searchIndex.IsEmpty())
			root.SetElementValue("SearchIndex", searchIndex.Serialize());

		xml.WriteToMemory(buffer);
	}
}