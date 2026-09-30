#include <pch.h>
#include "gui/UserSettingsEditorExtensionsTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/PackageWidget.h"
#include "io/PackageManager.h"
#include "data/Character.h"
#include "util/StringUtils.h"

using namespace fig::data;

namespace fig::gui
{
	UserSettingsEditorExtensionsTab::UserSettingsEditorExtensionsTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool UserSettingsEditorExtensionsTab::Initialize(UserSettingsEditorArgs args)
	{
		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Voice generation");
		CreateLabel(this, pSizer, "One of the packages below is required to enable voice features.");

		auto ttsServerPackages = Global::GetPackageManager().GetPackages()
			| std::views::filter([](auto&& p) { return p.type == PackageType::TTSServer; })
			| std::ranges::to<std::vector>();

		for (auto& package : ttsServerPackages)
		{
			auto pWidget = CreateControl<PackageWidget>(package);
			pSizer->Add(pWidget, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
			_widgets.push_back(pWidget);
		}

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Voice models");

		auto ttsVoiceModelPackages = Global::GetPackageManager().GetPackages()
			| std::views::filter([](auto&& p) { return p.type == PackageType::TTSVoiceModel; })
			| std::ranges::to<std::vector>();

		for (auto& package : ttsVoiceModelPackages)
		{
			auto pWidget = CreateControl<PackageWidget>(package);
			pSizer->Add(pWidget, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
			_widgets.push_back(pWidget);
		}

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Voice design models");

		auto ttsVoiceDesignPackages = Global::GetPackageManager().GetPackages()
			| std::views::filter([](auto&& p) { return p.type == PackageType::TTSDesignModel; })
			| std::ranges::to<std::vector>();

		for (auto& package : ttsVoiceDesignPackages)
		{
			auto pWidget = CreateControl<PackageWidget>(package);
			pSizer->Add(pWidget, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
			_widgets.push_back(pWidget);
		}
		return true;
	}

	void UserSettingsEditorExtensionsTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	EditorTabBase::SaveResult UserSettingsEditorExtensionsTab::OnSave() noexcept
	{
		return {};
	}
}