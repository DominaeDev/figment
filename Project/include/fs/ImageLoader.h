#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include <map>
#include <mutex>

namespace fig::io
{
	class ImageLoader
	{
	public:
		void Release();

		bool LoadCoverImage(const fig::uuid& characterAssetID);
		fig::uuid GetCoverImageID(const fig::uuid& characterAssetID) noexcept;
		fig::gui::TexturePtr GetTexture(fig::gui::RendererPtr pRenderer, const fig::uuid& assetID);

	private:
		std::map<fig::uuid, fig::uuid> _coverImages;
		std::map<fig::uuid, fig::sdl::Surface> _images;
		std::map<fig::uuid, fig::sdl::Texture> _textures;
	};
}