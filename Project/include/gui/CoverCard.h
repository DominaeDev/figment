#ifndef COVER_CARD_H__
#define COVER_CARD_H__

#pragma once

#include "Image.h"

namespace fig::gui
{
	class StaticText;

	class CoverCard : public Image
	{
	public:
		CoverCard(Control* pParent, const fig::uuid& assetId);

	protected:
		void OnRender(Renderer* pRenderer) override;

		void SetLabel(const fig::string& text) noexcept;
		void SetSublabel(const fig::string& text) noexcept;
		void CreateChatCounter(uint32_t count);

	private:
		fig::uuid _assetId;
		fig::sdl::Surface _surface;
		fig::sdl::Texture _texture;
		StaticText* _pLabel {};
		StaticText* _pSublabel {};
	};
}

#endif