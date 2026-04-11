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

		Count,
	};

	enum class SortBy
	{
		Name = 0,
		CreatedAt,
		UpdatedAt,
		MostRecentChat,

		Count,
	};

	enum class OrderBy
	{
		Ascending = 0,
		Descending,
		
		Count,
	};

	extern template class SettingsCollection<UserSetting>;
	using UserSettings = SettingsCollection<UserSetting>;
}

#endif
