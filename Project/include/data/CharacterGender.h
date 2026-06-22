#ifndef CHARACTER_GENDER_H__
#define CHARACTER_GENDER_H__

#pragma once

#include "Figment.h"

namespace fig::data
{
	enum class ConventionalGender
	{
		Undefined,
		Male,
		Female,
		Nonbinary,
		Newhalf,
	};

	enum class Pronouns
	{
		Undefined,
		Masculine,	// He/Him
		Feminine,	// She/Her
		Nonbinary,	// They/Them
		Neuter,		// It/It
	};

	class Gender
	{
	public:
		Gender() = default;
		explicit Gender(ConventionalGender gender);
		explicit Gender(const fig::string& gender);
		Gender(const Gender& value) = default;
		Gender(Gender&& value) = default;

		std::tuple<fig::string, ConventionalGender, Pronouns> Get() const noexcept;
		inline bool IsConventional() const noexcept { return _conventional != ConventionalGender::Undefined; };

		fig::string GetLabel() const noexcept { return _label; }
		Pronouns GetPronouns() const noexcept { return _pronouns; }

		Gender& operator= (const Gender& other) noexcept = default;
		Gender& operator= (Gender&& other) noexcept = default;
		Gender& operator= (fig::string_view gender) noexcept;
		Gender& operator= (ConventionalGender gender) noexcept;

		bool operator== (ConventionalGender gender) const noexcept;
		bool operator== (const fig::string& gender) const noexcept;
		bool operator!= (ConventionalGender gender) const noexcept { return !operator==(gender); }
		bool operator!= (const fig::string& gender) const noexcept { return !operator==(gender); }

		explicit operator fig::string() const { return GetLabel(); }
		explicit operator ConventionalGender() const { return _conventional; }

		static Gender Male;
		static Gender Female;
		static Gender Nonbinary;

	private:
		fig::string _label;
		ConventionalGender _conventional {};
		Pronouns _pronouns {};
	};
}
#endif