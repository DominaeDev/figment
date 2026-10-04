#pragma once

#include "gui/Editor.h"
#include "data/Character.h"

namespace fig::gui
{
	class CharacterEditor : public Editor
	{
	public:
		CharacterEditor(ControlPtr pParent);

		bool Initialize(const fig::uuid& assetId) noexcept;
		
		fig::string GetTitle() const noexcept override;
		std::vector<EditorTabDescriptor> GetTabDescriptors() const override;
		void PopulateTopBar(ControlPtr pTopBar) override;

		bool Save() noexcept;

	protected:
		void OnPropertyChanged() override;
		void SaveChanges();
		void DismissChanges();

	private:
		fig::observer_ptr<ThemedButton> _pSaveButton;
		fig::uuid _assetId {};
		fig::data::Character _character {};

	};
}