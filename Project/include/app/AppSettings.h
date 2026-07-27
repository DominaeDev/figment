#pragma once

#include "io/SettingsCollection.h"

namespace fig::io
{
	namespace AppSetting
	{
		constexpr fig::io::SettingKey LastUser				{ "Application", "LastUser" };
		constexpr fig::io::SettingKey SignedIn				{ "Application", "SignedIn" };
		
		namespace Interface
		{
			constexpr fig::io::SettingKey SmoothScrolling		{ "Interface", "SmoothScrolling" };
			constexpr fig::io::SettingKey WindowMaximized		{ "Interface", "WindowMaximized" };
			constexpr fig::io::SettingKey WindowSize			{ "Interface", "WindowSize" };
		}
	}

	class AppSettings : public SettingsCollection
	{
	public:
		explicit AppSettings(const fig::path& path) noexcept : SettingsCollection(path)
		{
			Init();
		}

		void Init() noexcept override;
		FileError Load() noexcept override;
		FileError Save() const noexcept override;
	};
}
