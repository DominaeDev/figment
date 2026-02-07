#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include <map>

namespace fig::gui
{
	class ImageStore
	{
	public:
		static void Init();
		static void Release();

		static bool LoadCoverImage(const fig::uuid& characterAssetID);
		static fig::uuid GetCoverImageID(const fig::uuid& characterAssetID) noexcept;
		static Texture* GetTexture(RendererPtr pRenderer, const fig::uuid& assetID);
	
	private:
		static std::map<fig::uuid, fig::uuid> _coverImages;
		static std::map<fig::uuid, fig::sdl::Surface> _images;
		static std::map<fig::uuid, fig::sdl::Texture> _textures;
	};
}