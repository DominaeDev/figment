#include <pch.h>
#include "gui/FolderWidget.h"
#include "gui/ButtonWithIcon.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TextInputDialogBox.h"
#include "gui/Menu.h"
#include "io/ContentManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr fig::coord LeftMargin = 8;
	constexpr fig::coord Spacing = 0;

	FolderWidget::FolderWidget(ControlPtr pParent) : Control(pParent)
	{
		auto pSizer = SetSizer<HorizontalSizer>();
	}

	void FolderWidget::SetFolder(const fig::uuid& folderId, AssetFolderType folderType)
	{
		if (folderId == _currentFolderId and not _children.empty())
			return;

		_currentFolderId = folderId;
		_folderType = folderType;
		_folders.clear();

		DestroyChildren();

		auto pSizer = GetSizer();
		pSizer->Clear();
		pSizer->AddSpacer(LeftMargin);
		
		_pHomeButton = CreateHomeButton();

		switch (folderType)
		{
		case AssetFolderType::Character:
			_pRootButton = CreateButton(fig::strings::UI::MenuCharacters, {});
			break;
		case AssetFolderType::Story:
			_pRootButton = CreateButton(fig::strings::UI::MenuScenarios, {});
			break;
		case AssetFolderType::World:
			_pRootButton = CreateButton(fig::strings::UI::MenuWorlds, {});
			break;
		default:
			assert(false);
			break;
		}

		fig::uuid currentFolderId = folderId;
		while (not currentFolderId.empty())
		{
			if (auto folder = Global::GetUserContent().GetFolder(currentFolderId))
			{
				auto pSeparator = CreateSeparator();
				auto pButton = CreateButton((*folder).name, (*folder).id);
				_folders.insert(_folders.begin(), FolderPiece {
					.pSeparator = pSeparator,
					.pButton = pButton,
				});
				currentFolderId = (*folder).parentId;
				continue;
			}
			break;
		}

		_folders.insert(_folders.begin(), FolderPiece {
			.pButton = _pRootButton,
		});

		for (auto& piece : _folders)
		{
			if (piece.pSeparator)
				pSizer->Add(piece.pSeparator, 0, SizerFlag::AlignCenterVertical | SizerFlag::Left, Spacing);
			if (piece.pButton)
				pSizer->Add(piece.pButton, 0, SizerFlag::AlignCenterVertical);
		}

		_pMenuButton = CreateMenuButton();

		LayoutNow();
		_folderWidth = _pMenuButton->GetRight() - LeftMargin;

		RefreshButtons();
	}

	static void NavigateToFolder(const fig::uuid& folderId)
	{
		PushEvent(UserEvent::ChangeAssetFolder, (fig::string)folderId);
	}

	fig::observer_ptr<ThemedButton> FolderWidget::CreateButton(fig::string_view label, const fig::uuid& folderId)
	{
		constexpr fig::coord MaxWidth = 300;
		auto pButton = CreateControl<ButtonWithLabel>(label);
		pButton->SetBorderRenderer(nullptr);
		pButton->SetTheme(ButtonThemes::TopBar);
		pButton->SetFont(FontFace::Italic, 24);
		pButton->SetDelegate([folderId] { PushEvent(UserEvent::ChangeAssetFolder, (fig::string)folderId); });

		if (auto pFont = pButton->GetFont())
		{
			if (auto [w, h] = MeasureText(*pFont, label); w <= MaxWidth)
				pButton->SetWidth(w + 8);
			else
			{
				auto truncated = TruncateTextByWidth(*pFont, label, MaxWidth);
				pButton->SetLabel(truncated);
				auto [ww, hh] = MeasureText(*pFont, truncated);
				pButton->SetWidth(ww + 8);
			}
		}
		return pButton;
	}

	fig::observer_ptr<Control> FolderWidget::CreateSeparator()
	{
		auto pLabel = CreateControl<StaticText>("/", FontFace::Italic, 24, false);
		pLabel->SetForegroundColor(Color::ButtonDefaultForeground);

		auto [w, h] = pLabel->MeasureText();
		pLabel->SetSize(w, 36);

		pLabel->SetAlignment(TextAlignment::LeftCenter);
		return pLabel;
	}

	fig::observer_ptr<ThemedButton> FolderWidget::CreateMenuButton()
	{
		auto pSizer = GetSizer();

		auto pButton = CreateControl<ButtonWithIcon>(Icon::CHEVRON_RIGHT_SMALL);
		pButton->SetBorderRenderer(nullptr);
		pButton->SetSize(20, 36);
		pButton->SetTheme(ButtonThemes::TopBar);
		pButton->SetDelegate([this] { ShowMenu(); });
		pSizer->Add(pButton, 0, SizerFlag::AlignCenterVertical);
		return pButton;
	}

	fig::observer_ptr<ThemedButton> FolderWidget::CreateHomeButton()
	{
		auto pSizer = GetSizer();

		auto pButton = CreateControl<ButtonWithIcon>(Icon::HOME);
		pButton->SetBorderRenderer(nullptr);
		pButton->SetSize(36, 36);
		pButton->SetTheme(ButtonThemes::TopBar);
		pButton->SetDelegate([this] { NavigateToFolder({}); });

		pSizer->Add(pButton, 0, SizerFlag::AlignCenterVertical);
		return pButton;
	}
	
	static void CreateFolderSubMenu(MenuItem& parentItem, const fig::uuid& folderId)
	{
		auto subfolders = Global::GetUserContent().GetSubFolders(folderId);
		for (auto& subfolder : subfolders)
		{
			auto& folderItem = parentItem.AddItem(subfolder.name, Icon::FOLDER)
				.SetDelegate([folderId = subfolder.id] { NavigateToFolder(folderId); });
			CreateFolderSubMenu(folderItem, subfolder.id);
		}
	}

	void FolderWidget::ShowMenu()
	{
		auto& menu = CreateMenu();
		menu.AddItem("New folder\u2026")
			.SetDelegate([this] {
				auto& dlg = ShowTextInputDialogBox("Please enter a name for the new folder:", "Enter folder name", [this](fig::string_view value) {
					if (not empty_or_whitespace(value))
					{
						if (auto newFolder = Global::GetUserContent().CreateFolder(value, _folderType, _currentFolderId))
							PushEvent(UserEvent::ChangeAssetFolder, (fig::string)(*newFolder).id);
					}
				});
				dlg.SetDefaultValue("New folder");
				dlg.SetMaxLength(64uz);
			});

		if (auto subfolders = Global::GetUserContent().GetSubFolders(_currentFolderId, _folderType); not subfolders.empty())
		{
			menu.AddSeparator();
			for (auto& subfolder : subfolders)
			{
				auto& folderItem = menu.AddItem(fig::string { subfolder.name }, Icon::FOLDER)
					.SetDelegate([folderId = subfolder.id] { NavigateToFolder(folderId); });
				CreateFolderSubMenu(folderItem, subfolder.id);
			}
		}

		if (not _currentFolderId.empty())
		{
			if (auto folder = Global::GetUserContent().GetFolder(_currentFolderId))
			{
				auto& folderName = (*folder).name;

				menu.AddSeparator();
				menu.AddItem("Rename this folder\u2026", Icon::EDIT)
					.SetDelegate([this, folderName] {
						auto& dlg = ShowTextInputDialogBox("Please enter a new name for the folder:", "Enter folder name", [this](fig::string_view value) {
							if (Global::GetUserContent().RenameFolder(_currentFolderId, value))
								RefreshPath();
						});
						dlg.SetDefaultValue(folderName);
						dlg.SetMaxLength(64uz);
					});
				menu.AddItem("Delete this folder\u2026", Icon::DELETE)
					.SetDelegate([this] { OnDeleteFolder(_currentFolderId); });
			}
		}
		menu.Show();
	}

	void FolderWidget::RefreshPath()
	{
		fig::uuid folderId = _currentFolderId;
		_currentFolderId = {};
		SetFolder(folderId, _folderType);
	}

	void FolderWidget::OnDeleteFolder(const fig::uuid& folderId)
	{
		ShowDialogBox("Are you sure you want to delete this folder?\nAll content will be moved to the parent folder.", DialogBoxType::ConfirmCancel,
			[folderId](bool bOk) {
				if (bOk)
				{
					if (auto folder = Global::GetUserContent().GetFolder(folderId))
					{
						fig::uuid parentFolderId = (*folder).parentId;
						if (Global::GetUserContent().DeleteFolder(folderId))
							PushEvent(UserEvent::ChangeAssetFolder, (fig::string)parentFolderId);
					}
				}
			});
		
	}

	void FolderWidget::OnSize()
	{
		if (auto width = GetWidth(); _lastWidth != width)
		{
			RefreshButtons();
			_lastWidth = width;
		}
	}

	void FolderWidget::RefreshButtons()
	{
		// Show as many folders as can fit
		auto maxWidth = GetWidth() - _pHomeButton->GetWidth() - _pMenuButton->GetWidth() - LeftMargin;
		int remainingWidth = maxWidth;

		for (int32_t i = toI(_folders.size()) - 1; i >= 0; i--)
		{
			auto& piece = _folders[i];
			auto thisWidth = piece.pButton->GetRight() - (piece.pSeparator ? piece.pSeparator->GetX() : piece.pButton->GetX());
			piece.bVisible = thisWidth <= remainingWidth;
			remainingWidth -= thisWidth;
		}

		for (int32_t i = 0; i < toI(_folders.size()) - 1; i++)
		{
			auto& piece = _folders[i];
			if (piece.pSeparator)
			{
				piece.pSeparator->SetVisible(piece.bVisible);
				piece.pSeparator->EnableLayout(piece.bVisible);
			}
			if (piece.pButton)
			{
				piece.pButton->SetVisible(piece.bVisible);
				piece.pButton->EnableLayout(piece.bVisible);
			}
		}
		InvalidateLayout();
	}

}