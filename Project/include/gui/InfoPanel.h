#pragma once

#include "gui/Panel.h"

namespace fig::chat
{
	class ChatSession;
}

namespace fig::gui
{
	class ImageViewport;
	class ResizeHandle;
	class CharacterDetailsPanel;

	class InfoPanel : public Panel
	{
	public:
		InfoPanel(ControlPtr pParent);

		void SetSession(const fig::chat::ChatSession& session);
		void Expand() noexcept;
		void Collapse() noexcept;

		void SetImage(const fig::uuid& assetId);
		void ClearImage() noexcept;

	protected:
		void OnAfterLayout() override;
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;

		void Resize(fig::coord size) noexcept;

	private:
		bool _bExpanded { true };

		fig::observer_ptr<ImageViewport> _pViewport;
		fig::observer_ptr<Control> _pBottomPanel;
		fig::observer_ptr<LayoutElement> _pGradient;
		fig::observer_ptr<ButtonWithIcon> _pCollapseButton;
		fig::observer_ptr<ResizeHandle> _pResizeHandle;
		fig::observer_ptr<CharacterDetailsPanel> _pCharacterDetails;

		fig::observer_ptr<Control> _pExpandedRoot;
		fig::observer_ptr<Control> _pCollapsedRoot;
	};
}