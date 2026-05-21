#include <pch.h>
#include "io/data/CharacterGender.h"

namespace fig::io
{
	static const fig::string kMale = "Male";
	static const fig::string kFemale = "Female";

	CharacterGender::CharacterGender(Gender value)
	{
		operator=(value);
	}

	CharacterGender::CharacterGender(const fig::string& value)
	{
		operator=(value);
	}

	CharacterGender& CharacterGender::operator= (Gender gender) noexcept
	{
		_gender = gender;
		if (gender != Gender::Other)
			_customName.clear();
		return *this;
	}

	CharacterGender& CharacterGender::operator= (const fig::string& name) noexcept
	{
		auto trimmed = trim(name);
		if (equals(trimmed, kMale, true))
		{
			_gender = Gender::Male;
			_customName.clear();
		}
		else if (equals(trimmed, kFemale, true))
		{
			_gender = Gender::Female;
			_customName.clear();
		}
		else if (not trimmed.empty())
		{
			for (auto& alt : AlternativeLabels)
			{
				if (equals(alt, trimmed, true))
				{
					trimmed = alt;
					break;
				}
			}
			_gender = Gender::Other;
			_customName = trimmed;
		}
		else
		{
			_gender = Gender::Undefined;
			_customName.clear();
		}
		return *this;
	}

	std::pair<CharacterGender::Gender, fig::string> CharacterGender::Get() const noexcept
	{
		switch (_gender)
		{
			case Gender::Male:
				return std::make_pair(Gender::Male, kMale);
			case Gender::Female:
				return std::make_pair(Gender::Female, kFemale);
			case Gender::Other:
				return std::make_pair(Gender::Other, _customName);
			default:
				return std::make_pair(Gender::Undefined, "");
		}
	}

	fig::string CharacterGender::GetLabel() const noexcept
	{
		auto [_, s] = Get();
		return std::move(s);
	}

	bool CharacterGender::operator== (const fig::string& gender) const noexcept
	{
		auto [g, s] = Get();
		return equals(s, trim(gender), true);
	}

	bool CharacterGender::operator== (Gender gender) const noexcept
	{
		return _gender == gender;
	}
}
