#pragma once

#include "gui/Editor.h"

namespace fig::gui
{
	class CharacterEditor : public Editor
	{
	public:
		CharacterEditor(ControlPtr pParent, const fig::uuid& assetId);
		void Shutdown() override {};
		
		fig::string GetTitle() const noexcept override;
		void PopulateTopBar(ControlPtr pTopBar) override;

		bool Save() noexcept;

	protected:
		void OnAfterLayout() override;

	private:
		fig::observer_ptr<Control> _pSaveButton;
	};
}