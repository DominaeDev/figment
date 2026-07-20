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
		EventResult OnEvent(Event& event) override;

	private:
		fig::observer_ptr<ImageViewport> _pViewport;
		fig::observer_ptr<LayoutElement> _pGradient;
		fig::observer_ptr<ButtonWithIcon> _pCollapseButton;
		bool _bExpanded { true };

		Control* _pExpandedRoot;
		Control* _pCollapsedRoot;
	};
}