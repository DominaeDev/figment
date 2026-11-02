#pragma once

#include "gui/Icon.h"

namespace fig::gui
{
	class TextureResource
	{
	public:
		constexpr TextureResource() noexcept :
			_ptr { nullptr }
		{
		}
		constexpr TextureResource(texture_ptr pTexture) noexcept :
			_ptr { pTexture }
		{
		}
		constexpr TextureResource(TextureResource&& other) noexcept :
			_ptr { other._ptr }
		{
		}
		TextureResource(Icon icon);

		constexpr TextureResource(const TextureResource&) = default;

		TextureResource& operator= (const TextureResource& other) = default;
		TextureResource& operator= (TextureResource&& other) noexcept
		{
			_ptr = other._ptr;
			return *this;
		};

		texture_ptr operator*() const noexcept { return _ptr.get(); }
		texture_ptr get() const noexcept { return _ptr.get(); }

		constexpr bool IsDefined() const noexcept { return static_cast<bool>(_ptr); }
		explicit operator bool() const noexcept { return static_cast<bool>(_ptr); }

	protected:
		fig::observer_ptr<SDL_Texture> _ptr;
	};
}