#include <pch.h>
#include "gui/CharacterDetailsPanel.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr fig::coord Margin = 6;

	CharacterDetailsPanel::CharacterDetailsPanel(ControlPtr pParent) : ScrollPanel(pParent)
	{
		_pHeader = CreateControl<StaticText>("Name", FontFace::Default, 18.5, false);
		_pHeader->EnableEllipsis(true);

		_pDescription = CreateControl<StaticText>("Name", FontFace::Default, 14.0, true);
		_pDescription->EnableWordWrap(true);
		_pDescription->SetText(
			"Consequatur vel blanditiis et rem. Repellendus quas optio autem et aliquam dolorum pariatur esse. Ex est eius fuga. Alias non officia qui officiis ducimus. Aut placeat sint nam reprehenderit temporibus sunt quae consectetur. Qui et voluptas eum neque pariatur et.\n"
			"\n"
			"Non saepe odit beatae non odit aut qui. Iusto et vero dolores ut voluptatibus voluptatem non. Ratione optio eveniet aspernatur architecto id incidunt voluptas.\n"
			"\n"
			"Provident dolorem saepe quod qui exercitationem consequatur voluptate non. Optio labore consequatur ipsa. A expedita dicta sunt enim quae. Quia omnis magni eius rem est et sed eligendi.");

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->AddSpacer(2);
		pTopSizer->Add(_pHeader, 0, Sizer::Expand | Sizer::Right | Sizer::Left, Margin);
		pTopSizer->Add(_pDescription, 0, Sizer::Expand | Sizer::Right | Sizer::Left, Margin);

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorder->SetColor(Color::LineColor);

		EnableClipping(true);
		EnableCulling(true);
	}

	void CharacterDetailsPanel::SetCharacter(const fig::data::Character& character)
	{

	}

	void CharacterDetailsPanel::OnAfterLayout()
	{
		if (_children.empty())
			_maxExtent = 0;
		else
		{
			auto& bottomItem = _children.back();
			_maxExtent = bottomItem->GetY() + bottomItem->GetHeight();
		}

		ScrollPanel::OnAfterLayout();
	}

	void CharacterDetailsPanel::OnSize()
	{
		_pHeader->SetMaxSize(GetWidth() - 2 * Margin, -1);
		_pHeader->InvalidateText();
		_pDescription->SetMaxSize(GetWidth() - 2 * Margin, -1);
		_pDescription->InvalidateText();
		
		ScrollPanel::OnSize();
	}

}