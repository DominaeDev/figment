#ifndef CHARACTER_GENDER_H__
#define CHARACTER_GENDER_H__

#pragma once

#include "Figment.h"

namespace fig::data
{
	class CharacterGender
	{
	public:
		enum Gender
		{
			Undefined = 0,
			Male,
			Female,
			Other,
		};

		CharacterGender() = default;
		explicit CharacterGender(Gender gender);
		explicit CharacterGender(const fig::string& gender);
		CharacterGender(const CharacterGender& value) = default;
		CharacterGender(CharacterGender&& value) = default;

		std::pair<Gender, fig::string> Get() const noexcept;
		inline bool IsDefined() const noexcept { return _gender != Gender::Undefined; };

		CharacterGender& operator= (const CharacterGender& other) noexcept = default;
		CharacterGender& operator= (CharacterGender&& other) noexcept = default;

		CharacterGender& operator= (const fig::string& gender) noexcept;
		CharacterGender& operator= (Gender gender) noexcept;

		bool operator== (Gender gender) const noexcept;
		bool operator== (const fig::string& gender) const noexcept;
		inline bool operator!= (Gender gender) const noexcept { return !operator==(gender); }
		inline bool operator!= (const fig::string& gender) const noexcept { return !operator==(gender); }

		inline operator fig::string() const { return GetLabel(); }
		inline explicit operator Gender() const { return _gender; }

		static constexpr std::array<fig::string_view, 10> AlternativeLabels {
			"Futanari", "Futa", "Shemale", "Trans", "Transsexual", "Transgender", "Asexual", "Newhalf", "Nonbinary", "Non-binary"
		};

	private:
		fig::string GetLabel() const noexcept;

		Gender _gender { Gender::Undefined };
		fig::string _customName;
	};
}
#endif