#include "model/Character.h"
#include "util/StringUtility.h"
#include "gui/Color.h"

#include <tinyxml2.h>

using namespace tinyxml2;

bool Character::LoadFromXml(string filename)
{
	XMLDocument xmlDoc;
	if (xmlDoc.LoadFile(filename.c_str()) != XML_SUCCESS)
		return false;

	XMLElement& root = *xmlDoc.FirstChildElement();

	// Identifier
	XMLElement* pID = root.FirstChildElement("ID");
	if (pID)
		id = string_util::trim(pID->GetText());

	// Name(s)
	XMLElement* pFirstName = root.FirstChildElement("FirstName");
	if (pFirstName)
		shortName = string_util::trim(pFirstName->GetText());

	XMLElement* pFullName = root.FirstChildElement("FullName");
	if (pFullName)
		fullName = string_util::trim(pFullName->GetText());

	if (id.empty())
		id = shortName;
	if (fullName.empty())
		fullName = shortName;


	// Portrait
	XMLElement* pImage = root.FirstChildElement("Image");
	if (pImage)
		portraitFilename = string_util::trim(pImage->GetText());

	// Read description
	XMLElement* pBrief = root.FirstChildElement("Brief");
	if (pBrief)
		brief = string_util::trim(pBrief->GetText());

	// Read description
	XMLElement* pDesc = root.FirstChildElement("Description");
	if (pDesc)
		description = string_util::trim(pDesc->GetText());

	// Read gender
	XMLElement* pGender = root.FirstChildElement("Gender");
	if (pGender)
	{
		string gender = string_util::trim(pGender->GetText());
		if (!gender.empty())
			properties.push_back(CharacterProperty { "gender", gender, "Gender" });
	}

	// Color
	bgColor = (Color)0;
	borderColor = (Color)0;
	XMLElement* pColor = root.FirstChildElement("Color");
	if (pColor)
	{
		borderColor = color_util::color_from_string(pColor->GetText());

		float h, s, v;
		color_util::color_to_hsv(borderColor, h, s, v);

		if (s > 0.0f)
			bgColor = color_util::hsv_to_color(h, 0.05f, std::clamp(v + 0.25f, 0.8f, 1.0f));
		else
			bgColor = color_util::hsv_to_color(h, 0.0f, std::clamp(v + 0.5f, 0.8f, 1.0f));
	}

	return !id.empty() && !shortName.empty();
}