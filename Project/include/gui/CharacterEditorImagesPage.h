#pragma once

#include "gui/EditorPage.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::gui
{
	class GridSizer;
	class CharacterPortraitImage;

	class CharacterEditorImagesPage : public EditorPage<CharacterEditorArgs>
	{
	public:
		CharacterEditorImagesPage(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;
		void ShutDown() noexcept {};
		bool Save() override;

	protected:
		void LoadImages();
		void SelectCover(size_t index);
		void RemovePortrait(size_t index);
		void MovePortraitUp(size_t index);
		void MovePortraitDown(size_t index);

		void OnUpdate(float fElapsed) override;
		void OnAfterLayout();
		void OpenFile();
		void OnOpenFile(const fig::path& filename);
		void OnClickedPortrait(ControlPtr pControl, int32_t button);

		static void SDLCALL OnFileDialogResult(void* userdata, const char* const* fileList, int filter);
	private:
		fig::uuid _characterId;
		fig::uuid _coverAssetId;
		fig::observer_ptr<GridSizer> _pImageGridSizer;
		fig::observer_ptr<Control> _pAddButton;
		std::queue<fig::path> _loadQueue;
		
		struct Portrait
		{
			fig::uuid assetId;
			fig::observer_ptr<CharacterPortraitImage> pControl;
			fig::sdl::Surface image;
			int32_t order = 0;
			bool isCover = false;
			fig::io::DataFormat format = fig::io::DataFormat::Undefined;
			fig::bytes data {};
		};
		bool EnsureImage(Portrait& portrait);
				
		std::vector<Portrait> _portraits;
		std::vector<fig::uuid> _removedImages;
	};
}