#pragma once

#include "gui/Control.h"

namespace fig::user
{
	struct UserProfile;
}

namespace fig::gui
{
	class ResizeHandle;
	class Editor;

	class SidePanelContent : public Control
	{
	protected:
		SidePanelContent(ControlPtr pParent) : Control(pParent)
		{}
	public:
		virtual void ShowExpanded() = 0;
		virtual void ShowCollapsed() = 0;
	};

	class SidePanel : public Control
	{
	public:
		SidePanel(ControlPtr pParent);
	
		void Expand() noexcept;
		void Collapse() noexcept;

		enum Mode
		{
			Invalid = 0,
			Main,
			Editor,
		};
		void SetMode(Mode mode);

		void SetEditor(fig::observer_ptr<fig::gui::Editor> pEditor);

	protected:
		void OnAfterLayout() override;
		void OnSize() override;

		EventResult OnEvent(fig::event& event) override;
		void Resize(fig::coord size) noexcept;

		template <typename T>
			requires std::derived_from<T, SidePanelContent>
		void SetContent()
		{
			DestroyChild(_pContent);
			auto pContent = CreateControl<T>();
			MoveChildToBottom(pContent);
			_pSizer->Add(pContent, -1, SizerFlag::Expand | SizerFlag::Fill);
			_bExpanded ? pContent->ShowExpanded() : pContent->ShowCollapsed();
			_pContent = pContent;
		}
	private:
		bool _bExpanded { true };
		Mode _mode {};
		
		fig::observer_ptr<SidePanelContent> _pContent {};
		fig::observer_ptr<Control> _pGradient {};
		fig::observer_ptr<ResizeHandle> _pResizeHandle {};
	};
}
