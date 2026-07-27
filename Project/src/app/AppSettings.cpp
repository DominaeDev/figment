#include <pch.h>
#include "app/AppSettings.h"
#include "io/IniFile.h"

namespace fig::io
{
	static const std::vector<SettingTuple> _AppSettings 
	{
		{ AppSetting::SignedIn,						false },
		{ AppSetting::LastUser,						"" },
		{ AppSetting::Interface::SmoothScrolling,	true },
		{ AppSetting::Interface::WindowMaximized,	false },
		{ AppSetting::Interface::WindowSize,		fig::point { Constants::GUI::WindowDefaultWidth, Constants::GUI::WindowDefaultHeight } },
	};

	void AppSettings::Init() noexcept
	{
		OnInit(_AppSettings);
	}

	FileError AppSettings::Load() noexcept
	{
		return OnLoad(_AppSettings);
	}

	FileError AppSettings::Save() const noexcept
	{
		return OnSave(_AppSettings);
	}
}