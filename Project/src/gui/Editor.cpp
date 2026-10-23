#include <pch.h>
#include "gui/Editor.h"
#include "gui/DialogBox.h"

namespace fig::gui
{
	Editor::Editor(ControlPtr pParent) : Control(pParent)
	{
		_pTabSizer = SetSizer<VerticalSizer>();
	}

	void Editor::SelectTab(size_t index)
	{
		for (size_t i = 0uz; i < _tabs.size(); ++i)
			EnableTab(_tabs[i], i == index);

		if (index < _tabs.size())
			_tabs[index]->OnTabSelected();

		InvalidateLayout();
		LayoutNow();
	}

	void Editor::EnableTab(EditorTabBase* pTab, bool bEnabled)
	{
		if (pTab)
		{
			pTab->EnableLayout(bEnabled);
			pTab->SetVisible(bEnabled);
			pTab->SetEnabled(bEnabled);
		}
	}

	void Editor::Close()
	{
		auto fnClose = [this] {
			for (size_t i = 0uz; i < _tabs.size(); ++i)
				_tabs[i]->OnClose();
			OnClose();
			PushEvent(UserEvent::NavigateToHome);
		};

		if (not IsDirty())
		{
			fnClose();
			return;
		}

		auto& dlg = CreateDialogBox("Discard unsaved changes?", DialogBoxType::ConfirmCancel,
			[this, fnClose](bool bConfirm) {
				if (bConfirm)
					fnClose();
			});
		dlg.Show();
	}

	void Editor::SetDirty() noexcept
	{
		_bIsDirty = true;
		OnPropertyChanged();
	}

	void Editor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	bool Editor::SaveTabs()
	{
		bool bOk = true;
		for (auto& tab : _tabs)
			bOk &= (bool)(tab->OnSave());
		return bOk;
	}
}