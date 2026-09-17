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
		void OnShutdown() noexcept override {}
		
		fig::string GetTitle() const noexcept override;
		std::vector<EditorTabDescriptor> GetTabDescriptors() const override;
		void PopulateTopBar(ControlPtr pTopBar) override;

		bool Save() noexcept;

	protected:
		void OnAfterLayout() override;

	private:
		fig::observer_ptr<Control> _pSaveButton;
		fig::uuid _assetId {};
		fig::data::Character _character {};

	};
}