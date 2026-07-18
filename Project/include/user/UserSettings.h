#pragma once

#include "io/Settings.h"

namespace fig
{
	enum class UserSetting
	{
		CharacterList_ShowTags,
		CharacterList_HalfSizeCards,
		CharacterList_Sorting,
		CharacterList_Ordering,
		CharacterList_Filtering,
		ChatList_Sorting,
		ChatList_Ordering,
		ChatList_Filtering,

		Clock,
		ModelPreset,

		Count,
	};

	extern template class fig::io::SettingsCollection<UserSetting>;
	using UserSettings = fig::io::SettingsCollection<UserSetting>;

	enum class SortBy
	{
		Name = 0,
		CreatedAt,
		UpdatedAt,
		LastUsedAt,
		ChatCount,

		Count,
		Default = UpdatedAt,
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
	
	enum class ChatFilterFlag : int32_t
	{
		Starred			= 1 << 1,
		Hidden			= 1 << 2,
	};
	using ChatFilterFlags = EnumFlags<ChatFilterFlag>;

	static auto ChatFilterFlagMapping = std::array<std::pair<ChatFilterFlag, std::string_view>, 3> {
		std::pair { ChatFilterFlag::Starred,	"starred" },
		std::pair { ChatFilterFlag::Hidden,		"hidden" },
	};

	constexpr ChatFilterFlags DefaultChatFilterFlags {};


}
