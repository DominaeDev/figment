#ifndef APPLICATION_SETTINGS_H__
#define APPLICATION_SETTINGS_H__
#pragma once

#include "io/Settings.h"

namespace fig
{
	enum class AppSetting
	{
		LastUser,
		SignedIn,
		WindowMaximized,
		WindowSize,
		SmoothScrolling,
	};

	extern template class fig::io::SettingsCollection<AppSetting>;
	using AppSettings = fig::io::SettingsCollection<AppSetting>;
}

#endif
