#include <pch.h>
#include "user/UserSettings.h"
#include "io/IniFile.h"
#include "gui/ColorTheme.h"

using namespace fig::gui;

namespace fig::io
{
	static const std::vector<SettingTuple> _UserSettings
	{
		{ UserSetting::Settings::TimeFormat,						"" },
		{ UserSetting::Settings::DateFormat,						"" },
		{ UserSetting::Settings::ModelPreset,						"" },

		{ UserSetting::Interface::Theme,							enum_serialize(ColorTheme::SystemDefault, ColorThemeMapping) },
		{ UserSetting::Interface::SidePanelCollapsed,				false },

		{ UserSetting::Interface::Chat::InfoPanelWidth,				Constants::GUI::InfoPanel::DefaultWidth },
		{ UserSetting::Interface::Chat::InfoPanelCollapsed,			false },
		{ UserSetting::Interface::Chat::ImageSize,					Constants::GUI::InfoPanel::DefaultImageSize },

		{ UserSetting::Interface::CharacterList::SmallCards,		false },
		{ UserSetting::Interface::CharacterList::ShowTags,			true },
		{ UserSetting::Interface::CharacterList::Sorting,			static_cast<int32_t>(SortBy::LastUsedAt) },
		{ UserSetting::Interface::CharacterList::Ordering,			static_cast<int32_t>(OrderBy::Default) },
		{ UserSetting::Interface::CharacterList::Filtering,			FilterFlags::Serialize(DefaultFilterFlags, FilterFlagMapping) },

		{ UserSetting::Interface::ChatList::Sorting,				static_cast<int32_t>(SortBy::Default) },
		{ UserSetting::Interface::ChatList::Ordering,				static_cast<int32_t>(OrderBy::Default) },
		{ UserSetting::Interface::ChatList::Filtering,				ChatFilterFlags::Serialize(DefaultChatFilterFlags, ChatFilterFlagMapping) },

		{ UserSetting::TTS::Enabled,								false },
		{ UserSetting::TTS::Warmup,									false },
		{ UserSetting::TTS::Split,									true },
		{ UserSetting::TTS::Volume,									0.8_fp },
		{ UserSetting::TTS::Backend,								"" },
		{ UserSetting::TTS::SpeechModel,							"" },
		{ UserSetting::TTS::DesignModel,							"" },
	};

	void UserSettings::Init() noexcept
	{
		OnInit(_UserSettings);
	}

	FileError UserSettings::Load() noexcept
	{
		return OnLoad(_UserSettings);
	}

	FileError UserSettings::Save() const noexcept
	{
		return OnSave(_UserSettings);
	}

	void UserSettings::OnSetDefaults()
	{
		// Auto-detect time and date formats
		if (auto format = TryGetEnum<DateFormat>(UserSetting::Settings::DateFormat, DateFormatMapping); not format.has_value())
		{
			SDL_DateFormat dateFormat {};
			if (SDL_GetDateTimeLocalePreferences(&dateFormat, NULL))
			{
				switch (dateFormat)
				{
				default:
				case SDL_DATE_FORMAT_YYYYMMDD:
					SetEnum(UserSetting::Settings::DateFormat, DateFormat::YYYYMMDD, DateFormatMapping);
					break;
				case SDL_DATE_FORMAT_DDMMYYYY:
					SetEnum(UserSetting::Settings::DateFormat, DateFormat::DDMMYYYY, DateFormatMapping);
					break;
				case SDL_DATE_FORMAT_MMDDYYYY:
					SetEnum(UserSetting::Settings::DateFormat, DateFormat::MMDDYYYY, DateFormatMapping);
					break;
				}
			}
		}

		if (auto format = TryGetEnum<TimeFormat>(UserSetting::Settings::TimeFormat, TimeFormatMapping); not format.has_value())
		{
			SDL_TimeFormat timeFormat {};
			if (SDL_GetDateTimeLocalePreferences(NULL, &timeFormat))
			{
				switch (timeFormat)
				{
				default:
				case SDL_TIME_FORMAT_24HR:
					SetEnum(UserSetting::Settings::TimeFormat, TimeFormat::HR24, TimeFormatMapping);
					break;
				case SDL_TIME_FORMAT_12HR:
					SetEnum(UserSetting::Settings::TimeFormat, TimeFormat::HR12, TimeFormatMapping);
					break;
				}
			}
		}
	}

	void UserSettings::SetChatListFilter(ChatFilterFlags filter)
	{
		SetFlags<ChatFilterFlag>(UserSetting::Interface::ChatList::Filtering, filter, ChatFilterFlagMapping);
	}

	ChatFilterFlags UserSettings::GetChatListFilter() const
	{
		return GetFlags<ChatFilterFlag>(UserSetting::Interface::ChatList::Filtering, DefaultChatFilterFlags, ChatFilterFlagMapping);
	}

	void UserSettings::SetColorTheme(fig::gui::ColorTheme theme)
	{
		SetEnum<ColorTheme>(UserSetting::Interface::Theme, theme, ColorThemeMapping);
	}

	fig::gui::ColorTheme UserSettings::GetColorTheme() const
	{
		return GetEnum<ColorTheme>(UserSetting::Interface::Theme, ColorThemeMapping, ColorTheme::SystemDefault);
	}

	DateFormat UserSettings::GetDateFormat() const
	{
		return GetEnum<DateFormat>(UserSetting::Settings::DateFormat, DateFormatMapping);
	}

	TimeFormat UserSettings::GetTimeFormat() const
	{
		return GetEnum<TimeFormat>(UserSetting::Settings::TimeFormat, TimeFormatMapping);
	}


}