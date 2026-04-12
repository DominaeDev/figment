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
		Filter,

		Count,
	};

	enum class SortBy
	{
		Name = 0,
		CreatedAt,
		UpdatedAt,
		LastMessaged,
		ChatCount,

		Count,
	};

	enum class OrderBy
	{
		Ascending = 0,
		Descending,
		
		Count,
	};

	enum class FilterFlag : int32_t
	{
		Male		= 1 << 0,
		Female		= 1 << 1,
		Other		= 1 << 2,

		New			= 1 << 4,
		Starred		= 1 << 5,
		Chats		= 1 << 6,
		Hidden		= 1 << 7,

		Exclusive	= New | Starred | Chats | Hidden,
	};
	using FilterFlags = EnumFlags<FilterFlag>;

	extern template class SettingsCollection<UserSetting>;
	using UserSettings = SettingsCollection<UserSetting>;
}

#endif
