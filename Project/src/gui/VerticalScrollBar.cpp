#include <pch.h>
#include "gui/VerticalScrollBar.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr Coord Margin = 8;

	VerticalScrollBar::VerticalScrollBar(LayoutElement* pParent) : Control(pParent)
	{
		SetBackgroundColor(Colors::Transparent);

		auto pHandle = new TexturedBorder(this, AppResources::GetTexture(TextureType::ROUNDED_BACKGROUND_6PX), 8);
		pHandle->SetWidth(8);
		pHandle->SetCornerScale(0.5f);
		pHandle->SetForegroundColor(Colors::Black.WithAlpha(0.25f));
		_pHandle = pHandle;
	}

	void VerticalScrollBar::SetScroll(float fPosition, Coord maxExtent)
	{
		_fPosition = fPosition;
		_fExtent = toF(maxExtent);
		_bDirty = true;
	}

	void VerticalScrollBar::OnUpdate(float fElapsed)
	{
		if (_bDirty)
		{
			RefreshHandle();
			_bDirty = false;
		}
	}

	void VerticalScrollBar::RefreshHandle()
	{
		int32_t pageHeight = GetHeight();
		float fScrollRange = _fExtent;
		if (fScrollRange <= 0.0f)
		{
			SetVisible(false);
			return;
		}
		
		SetVisible(true);

		int32_t handleSize = std::clamp(toI(std::min(toF(pageHeight) / fScrollRange, 1.0f) * pageHeight), 60, 300);
		_pHandle->CenterHorizontally();
		_pHandle->SetY(Margin + toI((_fPosition / _fExtent) * (pageHeight - handleSize - Margin * 2)));
		_pHandle->SetHeight(handleSize);
		_pHandle->CenterHorizontally();
	}

	bool VerticalScrollBar::OnEvent(Event& event)
	{
		return false;
	}
}