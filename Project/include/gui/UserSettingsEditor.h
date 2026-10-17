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

		fig::string GetTitle() const noexcept override;
		std::vector<EditorTabDescriptor> GetTabDescriptors() const override;
		void PopulateTopBar(ControlPtr pTopBar) override;

	protected:
		void Close() override;
		void OnPropertyChanged() override;

	private:
		fig::io::UserSettings _settings {};

	};
}