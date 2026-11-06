#pragma once

#include "gui/editor/EditorTab.h"
#include "gui/editor/WorldEditorArgs.h"

namespace fig::gui
{
	class GridSizer;
	class StoryBackgroundWidget;
	class NonOwningImageWithMask;

	class WorldEditorImagesTab : public EditorTab<WorldEditorArgs>
	{
	public:
		WorldEditorImagesTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(WorldEditorArgs args) override;

	protected:
		void ProcessLoadQueue();
		void OnUpdate(float fElapsed) override;

		void InitCover(SizerPtr pSizer);
		void OnClickedCover(ControlPtr pControl, int32_t button);
		void RemoveCover(size_t index);

		void InitBackgrounds(SizerPtr pSizer);
		void OnClickedBackground(ControlPtr pControl);
		void RemoveBackground(size_t index);
		void MoveBackgroundUp(size_t index);
		void MoveBackgroundDown(size_t index);

		enum ImageType { Cover, Background };
		void OpenFile(ImageType type);
		void OnOpenFile(const fig::path& filename, ImageType type);

		struct FileDialogUserData
		{
			WorldEditorImagesTab* pThis;
			ImageType type;
		} _fileDlgUserData {};
		static void SDLCALL OnFileDialogResult(void* userdata, const char* const* fileList, int filter);

	private:
		fig::uuid _worldId;
		std::vector<fig::uuid> _removedAssets;

		struct ImageWidget
		{
			fig::uuid assetId;
			fig::observer_ptr<Control> pControl;
			fig::sdl::Surface image;
			fig::io::DataFormat format = fig::io::DataFormat::Undefined;
			fig::bytes data {};
		};

		// Covers
		fig::observer_ptr<GridSizer> _pCoverGridSizer;
		fig::observer_ptr<Control> _pAddCoverButton;
		std::queue<fig::path> _coverLoadQueue;
		std::vector<ImageWidget> _coverWidgets;
		fig::uuid _coverAssetId;

		// Backgrounds
		fig::observer_ptr<GridSizer> _pBackgroundGridSizer;
		fig::observer_ptr<Control> _pAddBackgroundButton;
		std::queue<fig::path> _backgroundLoadQueue;
		std::vector<ImageWidget> _backgroundWidgets;
	};
}