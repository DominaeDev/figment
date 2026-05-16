#ifndef CHARACTER_GENDER_H__
#define CHARACTER_GENDER_H__

#pragma once

#include "Types.h"

namespace fig::io
{
	class CharacterGender
	{
	public:
		enum Gender
		{
			Undefined = 0,
			Male,
			Female,
			Other
		};

		CharacterGender() = default;
		explicit CharacterGender(Gender gender);
		explicit CharacterGender(const fig::string& gender);
		CharacterGender(const CharacterGender& value) = default;
		CharacterGender(CharacterGender&& value) = default;

		std::pair<Gender, fig::string> Get() const noexcept;
		fig::string GetName() const noexcept;
		inline bool IsDefined() const noexcept { return _gender != Gender::Undefined; };

		CharacterGender& operator= (const CharacterGender& other) noexcept = default;
		CharacterGender& operator= (CharacterGender&& other) noexcept = default;

		CharacterGender& operator= (const fig::string& gender) noexcept;
		CharacterGender& operator= (Gender gender) noexcept;

		bool operator== (Gender gender) const noexcept;
		bool operator== (const fig::string& gender) const noexcept;
		inline bool operator!= (Gender gender) const noexcept { return !operator==(gender); }
		inline bool operator!= (const fig::string& gender) const noexcept { return !operator==(gender); }

	private:
		Gender _gender { Gender::Undefined };
		fig::string _customName;
	};
}
#endif