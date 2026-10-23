#pragma once

#include "gui/Editor.h"
#include "data/Scenario.h"

namespace fig::gui
{
	class StoryEditor : public Editor
	{
	public:
		StoryEditor(ControlPtr pParent);

		bool NewStory() noexcept;
		bool EditCharacterStory(const fig::uuid& characterId) noexcept;
		bool EditStory(const fig::uuid& assetId) noexcept;

		fig::string GetTitle() const noexcept override;
		std::vector<EditorTabDescriptor> GetTabDescriptors() const override;
		void PopulateTopBar(ControlPtr pTopBar) override;

		bool Save() noexcept;
	protected:
		void OnPropertyChanged() override;
		void SaveChanges();
		void OnClose() override;

	private:
		fig::observer_ptr<ThemedButton> _pSaveButton;
		fig::uuid _assetId {};
		fig::data::Scenario _scenario {};
		bool _bEditing {};
	};
}