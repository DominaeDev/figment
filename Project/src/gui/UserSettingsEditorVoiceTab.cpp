#include <pch.h>
#include "gui/UserSettingsEditorVoiceTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "io/PackageManager.h"
#include "data/Character.h"
#include "util/StringUtils.h"

using namespace fig::data;

namespace fig::gui
{
	UserSettingsEditorVoiceTab::UserSettingsEditorVoiceTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool UserSettingsEditorVoiceTab::Initialize(UserSettingsEditorArgs args)
	{
		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Voice settings");

		// Notes
		CreateLabel(this, pSizer, "Voice server");
		
		auto pAddAttributeButton = CreateControl<ButtonWithLabel>("Install TTS Backend");
		pAddAttributeButton->SetHeight(35);
		pAddAttributeButton->SetDelegate([this] { InstallTTSServer(); });
		pSizer->Add(pAddAttributeButton, 0);

		CreateHint(this, pSizer, "Downloads and installs a local text-to-speech backend.");

		return true;
	}

	void UserSettingsEditorVoiceTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	EditorTabBase::SaveResult UserSettingsEditorVoiceTab::OnSave() noexcept
	{
		return {};
	}

	void UserSettingsEditorVoiceTab::InstallTTSServer()
	{
		Global::GetPackageManager().InstallPackage(fig::uuid { "eda3584f-a78f-4b7c-83dd-8f28fab23ea1" });
	}
}