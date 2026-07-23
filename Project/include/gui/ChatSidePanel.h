#pragma once

#include "gui/Panel.h"

namespace fig::gui
{
	class ImageViewport;

	class ChatSidePanel : public Panel
	{
	public:
		ChatSidePanel(ParentPtr pParent);

		void Expand() noexcept;
		void Collapse() noexcept;

		void SetImage(const fig::uuid& assetId);
		void ClearImage() noexcept;

	protected:
		void OnAfterLayout() override;
		EventResult OnEvent(fig::event& event) override;

	private:
		bool _bExpanded { true };

		fig::observer_ptr<ImageViewport> _pViewport;
		fig::observer_ptr<Control> _pBottomPanel;
		fig::observer_ptr<LayoutElement> _pGradient;
		fig::observer_ptr<ButtonWithIcon> _pCollapseButton;

		fig::observer_ptr<Control> _pExpandedRoot;
		fig::observer_ptr<Control> _pCollapsedRoot;
	};
}