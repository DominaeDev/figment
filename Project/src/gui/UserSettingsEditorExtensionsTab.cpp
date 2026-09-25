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

		auto ttsServerPackages = Global::GetPackageManager().GetPackages()
			| std::views::filter([](auto&& p) { return p.type == PackageType::TTS_Server; })
			| std::ranges::to<std::vector>();

		for (auto& package : ttsServerPackages)
		{
			auto pWidget = CreateControl<PackageWidget>(package);
			pSizer->Add(pWidget, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
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

	void UserSettingsEditorExtensionsTab::InstallTTSServer()
	{
		Global::GetPackageManager().InstallPackage(fig::uuid { "eda3584f-a78f-4b7c-83dd-8f28fab23ea1" });
	}
}