#ifndef USER_SETTINGS_H__
#define USER_SETTINGS_H__
#pragma once

#include "model/Settings.h"

namespace fig
{
	enum class UserSetting
	{
		ShowTags,
		HalfSizeCards,
		Sorting,
		Ordering,
		Filtering,
		
		ModelPreset,

		Count,
	};

	enum class SortBy
	{
		Name = 0,
		CreatedAt,
		UpdatedAt,
		LastUsedAt,
		ChatCount,

		Count,
		Default = LastUsedAt,
	};

	enum class OrderBy
	{
		Ascending = 0,
		Descending,
		
		Count,
		Default = Descending,
	};

	enum class FilterFlag : int32_t
	{
		GenderMale		= 1 << 0,
		GenderFemale	= 1 << 1,
		GenderOther		= 1 << 2,

		New				= 1 << 4,
		Starred			= 1 << 5,
		Chats			= 1 << 6,
		Hidden			= 1 << 7,
		SourceCreated	= 1 << 8,
		SourceImported	= 1 << 9,
	};
	using FilterFlags = EnumFlags<FilterFlag>;

	static auto FilterFlagMapping = std::array<std::pair<FilterFlag, std::string_view>, 9> {
		std::pair { FilterFlag::GenderMale,		"male" },
		std::pair { FilterFlag::GenderFemale,	"female" },
		std::pair { FilterFlag::GenderOther,	"nonbinary" },
		std::pair { FilterFlag::New,			"new" },
		std::pair { FilterFlag::Starred,		"starred" },
		std::pair { FilterFlag::Chats,			"chats" },
		std::pair { FilterFlag::Hidden,			"hidden" },
		std::pair { FilterFlag::SourceCreated,	"created" },
		std::pair { FilterFlag::SourceImported,	"imported" },
	};

	constexpr FilterFlags DefaultFilterFlags { FilterFlag::GenderMale, FilterFlag::GenderFemale, FilterFlag::GenderOther, FilterFlag::SourceCreated, FilterFlag::SourceImported  };
	extern template class SettingsCollection<UserSetting>;
	using UserSettings = SettingsCollection<UserSetting>;
}

#endif
