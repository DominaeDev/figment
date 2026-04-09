#include <pch.h>
#include "model/CharacterData.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"

using namespace fig::gui;
using namespace fig::util;
using namespace fig::io;

namespace fig::io
{
	static bool ReadXml(XmlReader& xml, CharacterData& data)
	{
		auto rootNode = xml.GetRootElement();

		// Identifier
		data.characterId = trim(rootNode.GetElementText("ID").value_or(""));

		// Name(s)
		data.fullName = trim(rootNode.GetElementText("FullName").value_or(""));
		data.shortName = trim(rootNode.GetElementText("FirstName").value_or(data.fullName));

		data.subheader = trim(rootNode.GetElementText("Subheader").value_or(""));

		if (data.characterId.empty())
			data.characterId = data.shortName;
		if (data.fullName.empty())
			data.fullName = data.shortName;

		// Portrait
		data.largePortraitFilename = trim(rootNode.GetElementText("LargePortrait").value_or(""));
		data.smallPortraitFilename = trim(rootNode.GetElementText("SmallPortrait").value_or(""));

		// Read brief
		data.brief = trim(rootNode.GetElementText("Brief").value_or(""));

		// Read description
		data.description = trim(rootNode.GetElementText("Description").value_or(""));

		// Read gender
		if (auto genderText = rootNode.GetElementText("Gender"))
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

		if (auto colorText = rootNode.GetElementText("Color"))
		{
			data.borderColor = Color::FromString(colorText.value());

			auto [h, s, v] = data.borderColor.GetHSV();

			if (s > 0.0f)
				data.bgColor = Color::FromHSV(h, 0.05f, std::clamp(v + 0.25f, 0.8f, 1.0f));
			else
				data.bgColor = Color::FromHSV(h, 0.0f, std::clamp(v + 0.5f, 0.8f, 1.0f));
		}

		// Tags
		data.tags = rootNode.GetElementList("Tags").value_or({});

		// Search
		data.searchIndex.Deserialize(rootNode.GetElementText("SearchIndex").value_or(""));

		return !data.characterId.empty() && !data.shortName.empty();
	}

	bool CharacterData::LoadFromXml(const fig::path& path)
	{
		XmlReader xml(path, "Character");
		if (not xml.IsOk())
			return false; // Invalid document type

		return ReadXml(xml, *this);
	}

	bool CharacterData::LoadFromXml(const fig::string& doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRootElement().GetName() != "Character")
			return false; // Invalid document type

		return ReadXml(xml, *this);
	}

	bool CharacterData::LoadFromXml(fig::string_view doc)
	{
		XmlReader xml(doc);
		if (not xml.IsOk() or xml.GetRootElement().GetName() != "Character")
			return false; // Invalid document type

		return ReadXml(xml, *this);
	}

	void CharacterData::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml("Character");

		auto root = xml.GetRoot();
		root.SetElementValue("ID", characterId);
		
		if (not shortName.empty())
			root.SetElementValue("FirstName", shortName);
		else
			root.SetElementValue("FirstName", "Unnamed");

		if (not fullName.empty())
			root.SetElementValue("FullName", fullName);

		if (not subheader.empty())
			root.SetElementValue("Subheader", subheader);

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

		if (not tags.empty())
			root.SetElementValue("Tags", tags);

		if (not brief.empty())
			root.SetElementValue("Brief", brief);
		if (not description.empty())
			root.SetElementValue("Description", description);
		if (not searchIndex.IsEmpty())
			root.SetElementValue("SearchIndex", searchIndex.Serialize());

		xml.SaveToMemory(buffer);
	}
}