#pragma once

#include "gui/GUITypes.h"
#include <map>

namespace fig::gui
{
	class TextureStore
	{
	public:
		static void Init(Renderer* pRenderer);
		static void Release();
		static TexturePtr GetTexture(TextureType id);
		static SurfacePtr GetImage(TextureType id) noexcept;
		static MaskPtr GetMask(MaskType maskId);

	private:
		static bool LoadTexture(Renderer* pRenderer, TextureType textureId, fig::path filename);
		static bool LoadTextureAndMaskCorners(Renderer* pRenderer, TextureType textureId, MaskType maskId, fig::path filename);
		static bool LoadMask(MaskType textureId, fig::path filename);

		static std::map<TextureType, fig::sdl::Surface> _surfaces;
		static std::map<TextureType, fig::sdl::Texture> _textures;
		static std::map<MaskType, Mask> _masks;
	};
}