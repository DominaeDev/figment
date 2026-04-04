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
	};

	extern template class SettingsCollection<UserSetting>;
	using UserSettings = SettingsCollection<UserSetting>;
}

#endif
