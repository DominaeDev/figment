#include <pch.h>
#include "user/UserSettings.h"
#include "io/IniFile.h"

namespace fig::io
{
	static const std::vector<SettingTuple> _UserSettings
	{
		{ UserSetting::Settings::Clock,								enum_serialize(Clock::Default, ClockMapping) },
		{ UserSetting::Settings::ModelPreset,						"" },

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
		{ UserSetting::TTS::Volume,									0.8_fp },
		{ UserSetting::TTS::TTSModel,								"" },
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
}