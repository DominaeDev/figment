#pragma once

#include "gui/Editor.h"
#include "user/UserSettings.h"

namespace fig::gui
{
	class UserSettingsEditor : public Editor
	{
	public:
		UserSettingsEditor(ControlPtr pParent);

		bool Initialize(const fig::uuid& assetId) noexcept;
		void OnShutdown() noexcept override {}

		fig::string GetTitle() const noexcept override;
		std::vector<EditorTabDescriptor> GetTabDescriptors() const override;
		void PopulateTopBar(ControlPtr pTopBar) override;

		bool Save() noexcept;

	protected:
		void OnAfterLayout() override;
		void OnPropertyChanged() override;

	private:
		fig::observer_ptr<ThemedButton> _pSaveButton;
		fig::io::UserSettings _settings {};

	};
}