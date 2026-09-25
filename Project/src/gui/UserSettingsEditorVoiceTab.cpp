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
}