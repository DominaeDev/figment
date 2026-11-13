#pragma once

#include "gui/editor/Editor.h"
#include "data/World.h"

namespace fig::gui
{
	class WorldEditor : public Editor
	{
	public:
		WorldEditor(ControlPtr pParent);

		bool NewWorld(const fig::uuid& folderId = {}) noexcept;
		bool EditWorld(const fig::uuid& assetId) noexcept;

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
		fig::data::World _world {};
		bool _bEditing {};
	};
}