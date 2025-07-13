#include "Character.h"
#include "tinyxml2.h"

using namespace tinyxml2;

bool Character::LoadFromXml(string filename)
{
	XMLDocument xmlDoc;
	if (xmlDoc.LoadFile(filename.c_str()) != XML_SUCCESS)
		return false;

	XMLElement& root = *xmlDoc.FirstChildElement();

	// Read name
	XMLElement* pName = root.FirstChildElement("Name");
	if (pName)
		name = pName->GetText();

	// Read description
	XMLElement* pDesc = root.FirstChildElement("Description");
	if (pDesc)
		description = pDesc->GetText();

	// Read gender
	XMLElement* pGender = root.FirstChildElement("Gender");
	if (pGender)
	{
		string gender = pGender->GetText();
		if (!gender.empty())
			properties.push_back(CharacterProperty { "gender", gender, "Gender" });
	}

	return !name.empty();
}