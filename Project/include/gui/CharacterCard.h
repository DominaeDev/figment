#ifndef CHARACTER_CARD_H__
#define CHARACTER_CARD_H__

#pragma once

#include "Image.h"

namespace fig::gui
{
	class CharacterCard : public Image
	{
	public:
		CharacterCard(Control* pParent);
		CharacterCard(Control* pParent, const fig::uuid& characterId);

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override;

	private:
		bool LoadCharacterPortrait();

	private:
		fig::uuid _characterId;
		fig::sdl::Surface _surface;
		fig::sdl::Texture _texture;
	};
}

#endif