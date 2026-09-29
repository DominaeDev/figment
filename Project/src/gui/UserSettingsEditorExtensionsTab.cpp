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
			_widgets.push_back(pWidget);

			pWidget->RefreshState();
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

	void UserSettingsEditorExtensionsTab::OnUpdate(float fElapsed)
	{
		static float fCounter = 0.0f;
		fCounter += fElapsed;
		if (fCounter > 0.3f)
		{
			fCounter = 0.0f;
			for (auto& pWidget : _widgets)
				pWidget->RefreshState();
		}
	}
}