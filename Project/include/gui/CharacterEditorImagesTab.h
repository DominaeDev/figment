#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::gui
{
	class GridSizer;
	class CharacterPortraitWidget;
	class CharacterSmallPortraitWidget;
	class CharacterBackgroundWidget;

	class CharacterEditorImagesTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorImagesTab(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;
		void ShutDown() noexcept {};
		SaveResult OnSave() noexcept override;

	protected:
		void InitSmallPortrait(SizerPtr pSizer);
		void InitPortraits(SizerPtr pSizer);
		void InitBackgrounds(SizerPtr pSizer);
		void SelectCover(size_t index);
		void SetSmallPortrait(size_t index);
		void ProcessLoadQueue();
		
		void OnUpdate(float fElapsed) override;
		void OnAfterLayout();

		void OnClickedSmallPortrait();
		void RevertSmallPortrait();

		void OnClickedPortrait(ControlPtr pControl, int32_t button);
		void RemovePortrait(size_t index);
		void MovePortraitUp(size_t index);
		void MovePortraitDown(size_t index);

		void OnClickedBackground(ControlPtr pControl);
		void RemoveBackground(size_t index);
		void MoveBackgroundUp(size_t index);
		void MoveBackgroundDown(size_t index);

		enum CharacterImageType { SmallPortrait, Portrait, Background };
		void OpenFile(CharacterImageType type);
		void OnOpenFile(const fig::path& filename, CharacterImageType type);

		struct FileDialogUserData
		{
			CharacterEditorImagesTab* pThis;
			CharacterImageType type;
		} _fileDlgUserData {};
		static void SDLCALL OnFileDialogResult(void* userdata, const char* const* fileList, int filter);
	private:
		fig::uuid _characterId;
		std::vector<fig::uuid> _removedAssets;
		
		struct ImageWidget
		{
			fig::uuid assetId;
			fig::observer_ptr<Control> pControl;
			fig::sdl::Surface image;
			bool isCover = false;
			fig::io::DataFormat format = fig::io::DataFormat::Undefined;
			fig::bytes data {};
		};

		// Small portrait
		fig::observer_ptr<CharacterSmallPortraitWidget> _pSmallPortrait;
		std::queue<fig::path> _smallPortraitLoadQueue;
		bool _bEditingSmallPortrait {};

		// Portraits
		fig::observer_ptr<GridSizer> _pPortraitGridSizer;
		fig::observer_ptr<Control> _pAddPortraitButton;
		std::queue<fig::path> _portraitLoadQueue;
		std::vector<ImageWidget> _portraitWidgets;
		fig::uuid _coverAssetId;

		// Backgrounds
		fig::observer_ptr<GridSizer> _pBackgroundGridSizer;
		fig::observer_ptr<Control> _pAddBackgroundButton;
		std::queue<fig::path> _backgroundLoadQueue;
		std::vector<ImageWidget> _backgroundWidgets;
	};
}