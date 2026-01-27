#include <pch.h>
#include "model/Character.h"
#include "gui/GUIUtility.h"
#include "util/StringUtility.h"
#include "util/Xml.h"

using namespace fig::gui;
using namespace fig::gui_util;
using namespace fig::string_util;

namespace fig::data
{
	bool Character::LoadFromXml(fig::string filename)
	{
		XmlReader xml(filename, "Character");
		if (not xml.IsOk())
			return false; // Invalid document type

		auto const& rootNode = xml.GetRootElement().value();

		// Identifier
		characterId = trim(rootNode.GetElementText("ID").value_or(""));

		// Name(s)
		shortName = trim(rootNode.GetElementText("FirstName").value_or(""));
		fullName = trim(rootNode.GetElementText("FullName").value_or(""));
		
		if (characterId.empty())
			characterId = shortName;
		if (fullName.empty())
			fullName = shortName;

		// Portrait
		portraitFilename = trim(rootNode.GetElementText("Image").value_or(""));

		// Read brief
		brief = trim(rootNode.GetElementText("Brief").value_or(""));

		// Read description
		description = trim(rootNode.GetElementText("Description").value_or(""));

		// Read gender
		if (auto genderText = rootNode.GetElementText("Gender"))
		{
			fig::string gender = trim(genderText.value());
			if (!gender.empty())
				properties.push_back(CharacterProperty { "gender", gender, "Gender" });
		}

		// Color
		bgColor = (Color)0;
		borderColor = (Color)0;

		if (auto colorText = rootNode.GetElementText("Color"))
		{
			borderColor = color_from_string(colorText.value());

			float h, s, v;
			color_to_hsv(borderColor, h, s, v);

			if (s > 0.0f)
				bgColor = hsv_to_color(h, 0.05f, std::clamp(v + 0.25f, 0.8f, 1.0f));
			else
				bgColor = hsv_to_color(h, 0.0f, std::clamp(v + 0.5f, 0.8f, 1.0f));
		}

		return !characterId.empty() && !shortName.empty();
	}
}