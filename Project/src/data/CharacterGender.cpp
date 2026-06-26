#include <pch.h>
#include "data/CharacterGender.h"

namespace fig::data
{
	Gender Gender::Male			{ "Male" };
	Gender Gender::Female		{ "Female" };
	Gender Gender::Nonbinary	{ "Non-binary" };

	constexpr std::array<std::tuple<ConventionalGender, fig::string_view, Pronouns, std::array<fig::string_view, 10>>, 4> ConventionalGenders
	{
		std::tuple { ConventionalGender::Male,			"Male",			Pronouns::Masculine,	std::array<fig::string_view, 10> { "male", "man" } },
		std::tuple { ConventionalGender::Female,		"Female",		Pronouns::Feminine,		std::array<fig::string_view, 10> { "female", "woman" } },
		std::tuple { ConventionalGender::Nonbinary,		"Non-binary",	Pronouns::Nonbinary,	std::array<fig::string_view, 10> { "nonbinary", "non-binary", "none", "non", "asexual", "trans", "transsexual", "trans-sexual", "trans-gender", "transgender" } },
		std::tuple { ConventionalGender::Newhalf,		"Futanari",		Pronouns::Feminine,		std::array<fig::string_view, 10> { "futanari", "futa", "shemale", "newhalf", "hermaphrodite", "herm", } },
	};

	static size_t find_conventional(string_view name)
	{
		if (name.empty())
			return npos;

		fig::handle normalized { name };
		for (size_t i = 0; i < ConventionalGenders.size(); ++i)
		{
			auto& labels = std::get<3>(ConventionalGenders[i]);
			if (auto itFind = std::ranges::find(labels, normalized, [](auto& value) { return fig::handle { value }; }); itFind != std::cend(labels))
				return i;
		}
		return npos;
	}

	static size_t find_conventional(ConventionalGender gender)
	{
		for (size_t i = 0; i < ConventionalGenders.size(); ++i)
		{
			if (gender == std::get<0>(ConventionalGenders[i]))
				return i;
		}
		return npos;
	}

	static ConventionalGender to_conventional(string_view name)
	{
		if (size_t conv_idx = find_conventional(name); conv_idx != npos)
			return std::get<0>(ConventionalGenders[conv_idx]);
		return ConventionalGender::Undefined;
	}

	Gender::Gender(ConventionalGender value)
	{
		operator=(value);
	}

	Gender::Gender(const fig::string& value)
	{
		operator=(value);
	}

	Gender& Gender::operator= (ConventionalGender gender) noexcept
	{
		_conventional = gender;
		if (size_t conv_idx = find_conventional(gender); conv_idx != npos)
		{
			const auto& conv = ConventionalGenders[conv_idx];
			_label = std::get<1>(conv);
			_pronouns = std::get<2>(conv);
		}
		return *this;
	}

	Gender& Gender::operator= (fig::string_view name) noexcept
	{
		_label = trim(name);

		if (size_t conv_idx = find_conventional(name); conv_idx != npos)
		{
			const auto& conv = ConventionalGenders[conv_idx];
			_conventional = std::get<0>(conv);
			_pronouns = std::get<2>(conv);
		}
		else
		{
			_conventional = ConventionalGender::Undefined;
			_pronouns = Pronouns::Nonbinary;
		}
		return *this;
	}

	std::tuple<fig::string, ConventionalGender, Pronouns> Gender::Get() const noexcept
	{
		return std::make_tuple(_label, _conventional, _pronouns);
	}

	bool Gender::operator== (const fig::string& name) const noexcept
	{
		if (IsConventional())
		{
			if (auto conv = to_conventional(name); conv != ConventionalGender::Undefined)
				return _conventional == conv;
		}

		return equals(_label, trim(name), true);
	}

	bool Gender::operator== (ConventionalGender gender) const noexcept
	{
		return _conventional == gender;
	}
}
