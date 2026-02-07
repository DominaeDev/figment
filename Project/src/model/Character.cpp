#include <pch.h>
#include "model/Character.h"
#include "gui/GUIUtility.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"

using namespace fig::gui;
using namespace fig::gui_util;
using namespace fig::string_util;

namespace fig::data
{
	static bool ReadXml(XmlReader& xml, CharacterData& data)
	{
		auto rootNode = xml.GetRootElement();

		// Identifier
		data.characterId = trim(rootNode.GetElementText("ID").value_or(""));

		// Name(s)
		data.shortName = trim(rootNode.GetElementText("FirstName").value_or(""));
		data.fullName = trim(rootNode.GetElementText("FullName").value_or(""));

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

			if (string_util::equals(gender, "male", true))
				data.gender = CharacterGender::Male;
			else if (string_util::equals(gender, "female", true))
				data.gender = CharacterGender::Female;
			else if (not gender.empty())
			{
				data.gender = CharacterGender::Custom;
				data.properties[Constants::CharacterProperties::Gender] = CharacterProperty { "Gender", gender };
			}
			else
				data.gender = CharacterGender::Undefined;
		}

		// Color
		data.bgColor = (Color)0;
		data.borderColor = (Color)0;

		if (auto colorText = rootNode.GetElementText("Color"))
		{
			data.borderColor = color_from_string(colorText.value());

			float h, s, v;
			color_to_hsv(data.borderColor, h, s, v);

			if (s > 0.0f)
				data.bgColor = hsv_to_color(h, 0.05f, std::clamp(v + 0.25f, 0.8f, 1.0f));
			else
				data.bgColor = hsv_to_color(h, 0.0f, std::clamp(v + 0.5f, 0.8f, 1.0f));
		}

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

	void CharacterData::SaveToXml(fig::bytes& buffer)
	{
		XmlWriter xml("Character");

		auto root = xml.GetRoot();
		root.SetElement("ID", characterId);
		
		if (not shortName.empty())
			root.SetElement("FirstName", shortName);
		else
			root.SetElement("FirstName", "Unnamed");

		if (not fullName.empty())
			root.SetElement("FullName", fullName);

		switch (gender)
		{
		case CharacterGender::Male:
			root.SetElement("Gender", "Male");
			break;
		case CharacterGender::Female:
			root.SetElement("Gender", "Female");
			break;
		case CharacterGender::Custom:
			if (properties.contains(Constants::CharacterProperties::Gender))
				root.SetElement("Gender", properties[Constants::CharacterProperties::Gender].value);
			break;
		default:
			break;
		}

		if (not brief.empty())
			root.SetElement("Brief", brief);
		if (not description.empty())
			root.SetElement("Description", description);

		xml.SaveToMemory(buffer);
	}
}