#ifndef APPLICATION_SETTINGS_H__
#define APPLICATION_SETTINGS_H__
#pragma once

#include "model/Settings.h"

namespace fig
{
	enum class AppSetting
	{
		LastUser,
		WindowMaximized,
		WindowSize,
	};

	extern template class SettingsCollection<AppSetting>;
	using AppSettings = SettingsCollection<AppSetting>;
}

#endif
