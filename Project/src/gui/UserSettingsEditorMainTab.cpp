#include <pch.h>
#include "gui/UserSettingsEditorMainTab.h"
#include "gui/CheckBox.h"
#include "gui/ColorTheme.h"
#include "util/CommonUtils.h"

using namespace fig::io;
using namespace fig::data;
using namespace fig::tts;

namespace fig::gui
{
	UserSettingsEditorMainTab::UserSettingsEditorMainTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool UserSettingsEditorMainTab::Initialize(UserSettingsEditorArgs args)
	{
		auto& userSettings = Global::GetUserSettings();
		auto pSizer = SetSizer<VerticalSizer>();

		// Color theme
		static constexpr std::array<std::pair<fig::string_view, ColorTheme>, 13uz> Themes
		{
			std::pair { "System default",	ColorTheme::SystemDefault },
			std::pair { "Vanilla",			ColorTheme::LightDefault },
			std::pair { "Soft ash",			ColorTheme::LightGray },
			std::pair { "Cherry blossom",	ColorTheme::LightPink },
			std::pair { "Cool blue",		ColorTheme::LightBlue },
			std::pair { "Morning dew",		ColorTheme::LightGreen },
			std::pair { "Lemon sorbet",		ColorTheme::LightYellow },
			std::pair { "Graphite",			ColorTheme::DarkDefault },
			std::pair { "Obsidian",			ColorTheme::DarkBlack },
			std::pair { "Cheeky rose",		ColorTheme::DarkPink  },
			std::pair { "Midnight blue",	ColorTheme::DarkBlue  },
			std::pair { "Forest green",		ColorTheme::DarkGreen },
			std::pair { "Espresso",			ColorTheme::DarkBrown },
		};

		CreateLabel(this, pSizer, "Color theme");
		auto themeNames = Themes
			| std::views::transform([](auto&& p) { return fig::string { p.first }; })
			| std::ranges::to<std::vector>();

		auto pTheme = CreateDropList(this, pSizer, themeNames, 
			[this](int32_t index) {
				if (index >= 0 and index < Themes.size())
					ChangeColorTheme(Themes[index].second);
			});
		pTheme->SetMaxWidth(260);
		if (auto index = find_index(Themes, Global::GetUserSettings().GetColorTheme(), [](auto&& p) { return p.second; }); index != fig::npos)
			pTheme->Select(toI(index), true);


		auto dateFormats = std::vector { 
			std::pair { DateFormat::YYYYMMDD, "YYYY-MM-DD"},
			std::pair { DateFormat::DDMMYYYY, "DD/MM/YYYY"},
			std::pair { DateFormat::MMDDYYYY, "MM/DD/YYYY"},
		};

		CreateLabel(this, pSizer, "Time and date format");
		auto pDateFormat = CreateControl<DropListOfType<DateFormat>>();
		for (auto& format : dateFormats)
			pDateFormat->AddItem(format.second, format.first);
		pDateFormat->SetDelegate([this, dateFormats](int32_t index) {
			if (index >= 0 and index < dateFormats.size())
				Global::GetUserSettings().SetEnum<DateFormat>(UserSetting::Settings::DateFormat, dateFormats[index].first, DateFormatMapping);
		});
		pDateFormat->SelectValue(Global::GetUserSettings().GetDateFormat());
		pDateFormat->SetMaxWidth(260);
		pSizer->Add(pDateFormat, 0, SizerFlag::Expand);

		auto timeFormats = std::vector {
			std::pair { TimeFormat::HR12, std::format("12 hour") },
			std::pair { TimeFormat::HR24, std::format("24 hour") },
		};

		auto pTimeFormat = CreateControl<DropListOfType<TimeFormat>>();
		for (auto& format : timeFormats)
			pTimeFormat->AddItem(format.second, format.first);
		pTimeFormat->SetDelegate([this, timeFormats](int32_t index) {
			if (index >= 0 and index < timeFormats.size())
				Global::GetUserSettings().SetEnum<TimeFormat>(UserSetting::Settings::TimeFormat, timeFormats[index].first, TimeFormatMapping);
		});
		pTimeFormat->SelectValue(Global::GetUserSettings().GetTimeFormat());
		pTimeFormat->SetMaxWidth(200);
		pSizer->AddSpacer(6);
		pSizer->Add(pTimeFormat, 0, SizerFlag::Expand);

		return true;
	}

	void UserSettingsEditorMainTab::ChangeColorTheme(ColorTheme theme)
	{
		Global::GetUserSettings().SetColorTheme(theme);
		AppColors::SetTheme(theme, true);
		PushEvent(UserEvent::ColorThemeChanged);
	}
}