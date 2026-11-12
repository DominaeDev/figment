#pragma once

#include "Control.h"
#include "io/AssetFolder.h"

namespace fig::gui
{
	class MenuItem;
	class FolderWidget : public Control
	{
	public:
		FolderWidget(ControlPtr pParent);

		void SetFolder(const fig::uuid& folderId, fig::io::AssetFolderType folderType);
		const fig::uuid& GetCurrentFolder() const noexcept { return _currentFolderId; }

		using ChangeFolderDelegate = std::function<void(fig::uuid)>;
		void SetDelegate(ChangeFolderDelegate fnDelegate) { _fnDelegate = fnDelegate; }

	protected:
		fig::observer_ptr<ThemedButton> CreateButton(fig::string_view label, const fig::uuid& folderId);
		fig::observer_ptr<Control> CreateSeparator();
		fig::observer_ptr<ThemedButton> CreateMenuButton();
		fig::observer_ptr<ThemedButton> CreateHomeButton();
		void RefreshButtons();

		void ShowMenu();
		void OnDeleteFolder(const fig::uuid& folderId);

		void OnSize() override;

	private:
		fig::uuid _currentFolderId;
		fig::io::AssetFolderType _folderType { fig::io::AssetFolderType::Undefined };
		ChangeFolderDelegate _fnDelegate;

		fig::observer_ptr<ThemedButton> _pHomeButton;
		fig::observer_ptr<ThemedButton> _pRootButton;
		fig::observer_ptr<ThemedButton> _pMenuButton;

		struct FolderPiece
		{
			fig::observer_ptr<Control> pSeparator;
			fig::observer_ptr<Control> pButton;
			bool bVisible { true };
		};
		std::vector<FolderPiece> _folders;
		fig::coord _folderWidth {};
		fig::coord _lastWidth {};
	};
}